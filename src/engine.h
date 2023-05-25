#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"
#include "graphics/renderer.h"

class Engine {

    Renderer renderer;

public:
    // Methods =========================
    int initialize(GLFWwindow *window);
    void clean();

    void render();
};
