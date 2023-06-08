#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"
#include "graphics/renderer.h"

class Engine {

    Renderer renderer;

public:

    int initialize(GLFWwindow *window, bool use_mirror_screen);
    void clean();

    bool get_openxr_available();
    bool get_use_mirror_window();

    void render();
};
