#include "display_manager.h"

#include "core/managers/debug/debug_manager.h"
#include "core/managers/engine/engine_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "GLFW/glfw3.h"
#include "core/managers/debug/debug_manager.h"

#ifdef __EMSCRIPTEN__
static EM_BOOL emscripten_resize_callback(int event_type,
        const EmscriptenUiEvent* ui_event, void* user_data)
{
    DisplayManager* display_server = reinterpret_cast<DisplayManager*>(user_data);
    display_server->window_set_size(ui_event->windowInnerWidth, ui_event->windowInnerHeight);
    return EM_TRUE;
}
#endif

void DisplayManager::glfw_resize_callback(GLFWwindow* window, int width, int height)
{
    // Minimized window
    if (width == 0 && height == 0) {
        return;
    }

    float dpi = DisplayManager::get_singleton()->window_get_dpi();
    int width_dpi = static_cast<int>(static_cast<float>(width) / dpi);
    int height_dpi = static_cast<int>(static_cast<float>(height) / dpi);
    DisplayManager::get_singleton()->window_set_size(width_dpi, height_dpi);

    if (!XR_MANAGER->is_xr_available()) {
        RENDER_MANAGER->set_render_size(width, height);
    }

    RENDER_MANAGER->resize_swapchain();
}

Error DisplayManager::initialize()
{
    singleton_instance = this;

    const sEngineConfig& config = EngineManager::get_singleton()->get_configuration();
    const bool use_xr = XR_MANAGER->is_xr_available();

    if (!glfwInit()) {
        LOG_ERROR("Could not initialize GLFW");
        return Error::FAILED;
    }

    GLFWmonitor* monitor = nullptr;

#ifdef __EMSCRIPTEN__
    window_width = canvas_get_width();
    window_height = canvas_get_height();

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, (void*)this, false, emscripten_resize_callback);
#else
    if (config.fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        window_width = mode->width;
        window_height = mode->height;
    } else {
        window_width = config.window_width;
        window_height = config.window_height;
    }
#endif

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

    LOG_INFO("Window size: {}x{}", window_width, window_height);

    glfwSetWindowTitle(window, config.window_title.c_str());

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

    LOG_INFO("Window size: {}x{}", window_width, window_height);
}

glm::ivec2 DisplayManager::window_get_framebuffer_size()
{
    glm::ivec2 framebuffer_size;
    glfwGetFramebufferSize(window, &(framebuffer_size.x), &(framebuffer_size.y));
    return framebuffer_size;
}

bool DisplayManager::window_is_minimized() const
{
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0;
}

bool DisplayManager::window_should_close() const
{
    return glfwWindowShouldClose(window);
}
