#include "display_manager.h"

#include "core/managers/engine/engine_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "GLFW/glfw3.h"
#include "spdlog/spdlog.h"

#ifdef __EMSCRIPTEN__
static EM_BOOL emscripten_resize_callback(int event_type,
        const EmscriptenUiEvent* ui_event, void* user_data)
{
    DisplayManager* display_server = reinterpret_cast<DisplayManager*>(user_data);
    display_server->window_set_size(ui_event->windowInnerWidth, ui_event->windowInnerHeight);
    return EM_TRUE;
}
#endif

void glfw_resize_callback(GLFWwindow* window, int width, int height)
{
    // Minimized window
    if (width == 0 && height == 0) {
        return;
    }

    spdlog::info("GLFW Window resized to ({}, {})", width, height);

    if (!XRManager::get_singleton()->is_xr_available()) {
        DisplayManager::get_singleton()->window_set_size(width, height);
    }
}

Error DisplayManager::initialize()
{
    singleton_instance = this;

    const sEngineConfig& config = EngineManager::get_singleton()->get_configuration();
    const bool use_xr = XRManager::get_singleton()->is_xr_available();

    GLFWmonitor* monitor = nullptr;

#ifdef __EMSCRIPTEN__
    window_width = canvas_get_width();
    window_height = canvas_get_height();

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, (void*)this, false, emscripten_resize_callback);
#else
    if (use_xr) {
        // Keep XR aspect ratio
        window_width = 992;
        window_height = 1000;
    } else {
        if (config.fullscreen) {
            monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            window_width = mode->width;
            window_height = mode->height;
        }

        window_width = config.window_width;
        window_height = config.window_height;
    }
#endif

    if (!glfwInit()) {
        spdlog::error("Could not initialize GLFW");
        return Error::FAILED;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

#ifndef __EMSCRIPTEN__
    float x_scale, y_scale;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &x_scale, &y_scale);
    dpi_scale = x_scale;
#else
    dpi_scale = emscripten_get_device_pixel_ratio();
#endif

    window = glfwCreateWindow(window_width, window_height, config.window_title.c_str(), monitor, nullptr);

    int display_width, display_height;
    //glfwGetWindowSize(window, &window_width, &window_height);
    glfwGetFramebufferSize(window, &display_width, &display_height);

    RenderManager::get_singleton()->set_render_size(display_width, display_height);

    glfwSetWindowTitle(window, config.window_title.c_str());

    spdlog::info("Window size: {}x{}", display_width, display_height);

#ifndef __EMSCRIPTEN__
    glfwSetFramebufferSizeCallback(window, glfw_resize_callback);
#endif

    return Error::OK;
}

Error DisplayManager::finalize()
{
    glfwDestroyWindow(window);
    glfwTerminate();

    singleton_instance = nullptr;
    return Error::OK;
}

void DisplayManager::process_events()
{
    glfwPollEvents();
}

GLFWwindow* DisplayManager::get_window() const
{
    return window;
}

void DisplayManager::window_set_size(int32_t width, int32_t height)
{
    window_width = width;
    window_height = height;
}

bool DisplayManager::window_is_minimized() const
{
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0;
}

bool DisplayManager::window_should_close() const
{
    return glfwWindowShouldClose(window);
}
