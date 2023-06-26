#pragma once

#include "utils.h"

#include "framework/mesh.h"

#include "graphics/shader.h"
#include "graphics/webgpu_context.h"

#ifdef XR_SUPPORT
#include "xr/openxr_context.h"
#endif

class Renderer {

protected:
#ifdef XR_SUPPORT
    OpenXRContext           xr_context;
#endif

    WebGPUContext           webgpu_context;

    uint32_t render_width   = 0;
    uint32_t render_height  = 0;

    bool is_openxr_available    = false;
    bool use_mirror_screen      = false;

public:

    Renderer();

    virtual int initialize(GLFWwindow* window, bool use_mirror_screen = false);
    virtual void clean();
    
    virtual void update(double delta_time) {};
    virtual void render() {};

    bool get_openxr_available() { return is_openxr_available; }
    bool get_use_mirror_screen() { return use_mirror_screen; }
    OpenXRContext* get_openxr_context() { return (is_openxr_available ? &xr_context : nullptr); }
};
