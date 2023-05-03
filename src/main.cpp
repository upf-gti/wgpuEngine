#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "utils.h"
#include "wgpu_environment.h"

#define WGPU_TARGET_MACOS 1
#define WGPU_TARGET_LINUX_X11 2
#define WGPU_TARGET_WINDOWS 3
#define WGPU_TARGET_LINUX_WAYLAND 4

#if WGPU_TARGET == WGPU_TARGET_MACOS
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

#include <GLFW/glfw3.h>
#if WGPU_TARGET == WGPU_TARGET_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
#elif WGPU_TARGET == WGPU_TARGET_LINUX_X11
#define GLFW_EXPOSE_NATIVE_X11
#elif WGPU_TARGET == WGPU_TARGET_LINUX_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif WGPU_TARGET == WGPU_TARGET_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

GLFWwindow* window = NULL;
WGPUEnv::sInstance wgpu_instance = {};

void main_render_loop() {
    wgpu_instance._config_render_pipeline();
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        wgpu_instance.render_frame();
    }
    wgpu_instance.clean();
}

int main() {
    if(!glfwInit()) {
        // Quit
        std::cout << "NO!" << std::endl;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640, 480, "WebGPU", NULL, NULL);
    
    wgpu_instance.initialize(window, NULL);

    // Wait until all the async stuff has been done
    while(!glfwWindowShouldClose(window) && !wgpu_instance.is_initialized) {};
    std::cout << "Finished config" << std::endl;
    main_render_loop();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}