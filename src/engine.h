#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"
#include "openxr_context.h"
#include "webgpu_context.h"

class Engine {

    OpenXRContext xr_context;
    WebGPUContext webgpu_context;

public:
    // Methods =========================
    int initialize(GLFWwindow *window);
    void clean();

    void render_frame();
    void renderXr(int swapchain_index);
    void renderMirror();
};