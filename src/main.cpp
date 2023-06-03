#include "engine.h"

#include <GLFW/glfw3.h>

void closeWindow(GLFWwindow* window) {
#if !defined(XR_SUPPORT) || (defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW))
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
}

int main() {

    Engine engine;
    GLFWwindow* window = nullptr;

    const bool use_xr = engine.isOpenXRAvailable();
    const bool use_mirror_screen = engine.useMirrorWindow();

    // Only init glfw if no xr or using mirror
    const bool use_glfw = !use_xr || (use_xr && use_mirror_screen);

    if (use_glfw) {
        if (!glfwInit()) {
            return 1;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            window = glfwCreateWindow(1280, 720, "WebGPU", NULL, NULL);
    }

    if (engine.initialize(window, use_mirror_screen)) {
        std::cout << "Could not initialize engine" << std::endl;
        closeWindow(window);
        return 1;
    }

    std::cout << "Engine initialized" << std::endl;

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

    engine.clean();

    closeWindow(window);

    return 0;
}