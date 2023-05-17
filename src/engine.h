#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"
#include "openxr_context.h"
#include "webgpu_context.h"

#define USE_MIRROR_WINDOW

class Engine {

    OpenXRContext xr_context;
    WebGPUContext webgpu_context;

public:
    // Methods =========================
    int initialize(GLFWwindow *window);
    void clean();

    void render_frame();
};
