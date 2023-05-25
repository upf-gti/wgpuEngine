#include "engine.h"

#include <GLFW/glfw3.h>

void closeWindow(GLFWwindow* window) {
#ifdef USE_MIRROR_WINDOW
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
}

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

    if (engine.initialize(window)) {
        std::cout << "Could not initialize engine" << std::endl;
        closeWindow(window);
        return 1;
    }

    // Wait until all the async stuff has been done
#ifdef USE_MIRROR_WINDOW
    //while(!glfwWindowShouldClose(window) && !wgpu_instance.is_initialized) {};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        engine.render();
    }
#else
    //while (!wgpu_instance.is_initialized) {};
    while (true) {
        engine.render();
    }
#endif

    engine.clean();

    closeWindow(window);

    return 0;
}