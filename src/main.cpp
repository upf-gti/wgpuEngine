#include "engine.h"

#include <GLFW/glfw3.h>

int main() {

    Engine engine;
    GLFWwindow* window = nullptr;

#ifdef USE_MIRROR_WINDOW
    if(!glfwInit()) {
        // Quit
        std::cout << "NO!" << std::endl;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640, 480, "WebGPU", NULL, NULL);
#endif

    engine.initialize(window);

    // Wait until all the async stuff has been done
#ifdef USE_MIRROR_WINDOW
    //while(!glfwWindowShouldClose(window) && !wgpu_instance.is_initialized) {};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        engine.render_frame();
    }
#else
    //while (!wgpu_instance.is_initialized) {};
    while (true) {
        wgpu_instance.render_frame();
    }
#endif

    engine.clean();

#ifdef USE_MIRROR_WINDOW
    glfwDestroyWindow(window);
    glfwTerminate();
#endif

    return 0;
}