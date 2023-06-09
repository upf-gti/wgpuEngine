#pragma once

#include "utils.h"

#include "graphics/mesh.h"
#include "graphics/shader.h"
#include "graphics/pipeline.h"
#include "graphics/webgpu_context.h"

#ifdef XR_SUPPORT
#include "xr/openxr_context.h"
#endif

#include "framework/entities/entity_mesh.h"

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

    // Entities to be rendered this frame
    std::vector < EntityMesh* > render_list;

public:

    // Singleton
    static Renderer* instance;

    Renderer();

    virtual int initialize(GLFWwindow* window, bool use_mirror_screen = false);
    virtual void clean();
    
    virtual void update(float delta_time) = 0;
    virtual void render() = 0;

    void add_renderable(EntityMesh *entity);

    bool get_openxr_available() { return is_openxr_available; }
    bool get_use_mirror_screen() { return use_mirror_screen; }

#ifdef XR_SUPPORT
    OpenXRContext* get_openxr_context() { return (is_openxr_available ? &xr_context : nullptr); }
#endif

};
