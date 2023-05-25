#include "engine.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "utils.h"

int Engine::initialize(GLFWwindow *window) {
    return renderer.initialize(window);
}

void Engine::clean() {
    renderer.clean();
}

void Engine::render() {

    renderer.render();
}
