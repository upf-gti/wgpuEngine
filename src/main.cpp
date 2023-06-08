#include "engine.h"

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
EM_JS(int, canvas_get_width, (), {
  return canvas.clientWidth;
});

EM_JS(int, canvas_get_height, (), {
  return canvas.clientHeight;
});
#endif

void closeWindow(GLFWwindow* window) {
#if !defined(XR_SUPPORT) || (defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW))
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
}

int main() {

    Engine engine;
    GLFWwindow* window = nullptr;

#ifdef __EMSCRIPTEN__
    int render_width = canvas_get_width();
    int render_height = canvas_get_height();
#else
    int render_width = 1280;
    int render_height = 720;
#endif

    const bool use_xr = engine.get_openxr_available();
    const bool use_mirror_screen = engine.get_use_mirror_window();

    // Only init glfw if no xr or using mirror
    const bool use_glfw = !use_xr || (use_xr && use_mirror_screen);

    if (use_glfw) {
        if (!glfwInit()) {
            return 1;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(render_width, render_height, "WebGPU Engine", NULL, NULL);
    }

    if (engine.initialize(window, use_mirror_screen)) {
        std::cout << "Could not initialize engine" << std::endl;
        closeWindow(window);
        return 1;
    }

    std::cout << "Engine initialized" << std::endl;

#ifdef __EMSCRIPTEN__

    emscripten_set_main_loop_arg(
        [](void* userData) {
            Engine& engine = *reinterpret_cast<Engine*>(userData);
            engine.render();
        },
        (void*)&engine,
        0, true
    );

#else

    if (use_glfw) {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            engine.render();
        }
    }
    else {
        while (true) {
            engine.render();
        }
    }

#endif

    engine.clean();

    closeWindow(window);

    return 0;
}