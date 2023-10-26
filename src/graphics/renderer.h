#pragma once

#include "utils.h"

#include "graphics/mesh.h"
#include "graphics/shader.h"
#include "graphics/pipeline.h"
#include "graphics/webgpu_context.h"
#include "graphics/renderer_storage.h"

#ifdef XR_SUPPORT
#include "xr/openxr_context.h"
#endif

#include "framework/entities/entity_mesh.h"

class EntityMesh;

class Renderer {

protected:
#ifdef XR_SUPPORT
    OpenXRContext   xr_context;
#endif

    WebGPUContext   webgpu_context;

    RendererStorage renderer_storage;

    bool is_openxr_available    = false;
    bool use_mirror_screen      = false;

    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Required device limits
    WGPURequiredLimits required_limits = {};

    struct sUniformData {
        glm::mat4x4 model;
        glm::vec4   color;
    };

    struct sRenderData {
        EntityMesh* entity_mesh;
        uint32_t repeat;
    };

    enum eRenderListType {
        RENDER_LIST_OPAQUE,
        RENDER_LIST_UI,
        RENDER_LIST_ALPHA,
        RENDER_LIST_SIZE
    };

    std::vector<sUniformData> instance_data[RENDER_LIST_SIZE];
    Uniform	instance_data_uniform[RENDER_LIST_SIZE];

    std::vector<RendererStorage::sUIData> instance_ui_data;
    Uniform	instance_ui_data_uniform;

    // Entities to be rendered this frame
    std::vector<sRenderData> render_list[RENDER_LIST_SIZE];

    // Bind group per shader
    WGPUBindGroup bind_groups[RENDER_LIST_SIZE] = {};

public:

    // Singleton
    static Renderer* instance;

    Renderer();

    virtual int initialize(GLFWwindow* window, bool use_mirror_screen = false);
    virtual void clean();
    
    virtual void update(float delta_time) = 0;
    virtual void render() = 0;

    void render(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);

    bool get_openxr_available() { return is_openxr_available; }
    bool get_use_mirror_screen() { return use_mirror_screen; }

    glm::vec4 get_clear_color() { return clear_color; }

#ifdef XR_SUPPORT
    OpenXRContext* get_openxr_context() { return (is_openxr_available ? &xr_context : nullptr); }
#endif
    WebGPUContext* get_webgpu_context() { return &webgpu_context; }

    void set_required_limits(const WGPURequiredLimits& required_limits) { this->required_limits = required_limits; }

    void add_renderable(EntityMesh* entity_mesh);
    void clear_renderables();

    virtual void resize_window(int width, int height);
};
