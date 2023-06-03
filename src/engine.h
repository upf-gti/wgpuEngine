#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"
#include "graphics/renderer.h"

class Engine {

    Renderer renderer;

public:

    int initialize(GLFWwindow *window, bool use_mirror_screen);
    void clean();

    bool isOpenXRAvailable();
    bool useMirrorWindow();

    void render();
};
