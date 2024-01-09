#pragma once

#include "framework/utils/utils.h"

#include "graphics/shader.h"
#include "graphics/pipeline.h"
#include "graphics/webgpu_context.h"
#include "graphics/renderer_storage.h"
#include "graphics/surface.h"

#ifdef XR_SUPPORT
#include "xr/openxr_context.h"
#endif

#include "framework/entities/entity_mesh.h"

#include "graphics/debug/renderdoc_capture.h"

class EntityMesh;
class Camera;

class Renderer {

protected:
#ifdef XR_SUPPORT
    OpenXRContext   xr_context;
#endif

    WebGPUContext   webgpu_context;

    Camera* camera = nullptr;
    Texture* irradiance_texture = nullptr;

    RendererStorage renderer_storage;

#ifndef __EMSCRIPTEN__
    RenderdocCapture renderdoc_capture;
#endif

    bool is_openxr_available    = false;
    bool use_mirror_screen      = false;

    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    float z_near = 0.01f;
    float z_far = 1000.0f;

    // Required device limits
    WGPURequiredLimits required_limits = {};

    struct sUniformData {
        glm::mat4x4 model;
        glm::vec4   color;
    };

    struct sRenderData {
        Surface* surface;
        uint32_t repeat;
        glm::mat4x4 global_matrix;
        EntityMesh* entity_mesh_ref;
    };

    enum eRenderListType {
        RENDER_LIST_OPAQUE,
        RENDER_LIST_UI,
        RENDER_LIST_ALPHA,
        RENDER_LIST_SIZE
    };

    void render_render_list(int list_index, WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);

    std::vector<sUniformData> instance_data[RENDER_LIST_SIZE];
    Uniform	instance_data_uniform[RENDER_LIST_SIZE];

    std::vector<RendererStorage::sUIData> instance_ui_data;
    Uniform	instance_ui_data_uniform;

    // Entities to be rendered this frame
    std::vector<EntityMesh*> render_entity_list;

    std::vector<sRenderData> render_list[RENDER_LIST_SIZE];

    // Bind group per shader
    WGPUBindGroup bind_groups[RENDER_LIST_SIZE] = {};

    // Bind group for image based lighting
    WGPUBindGroup ibl_bind_group;
    Uniform irradiance_texture_uniform;
    Uniform brdf_lut_uniform;
    Uniform ibl_sampler_uniform;

public:

    // Singleton
    static Renderer* instance;

    Renderer();

    virtual int initialize(GLFWwindow* window, bool use_mirror_screen = false);
    virtual void clean();
    
    virtual void update(float delta_time) = 0;
    virtual void render() = 0;

    void init_ibl_bind_group();
    WGPUBindGroup get_ibl_bind_group() { return ibl_bind_group; }

    void prepare_instancing();
    void render_opaque(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);
    void render_transparent(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);

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

    GLFWwindow* get_glfw_window() { return webgpu_context.window; };

    Camera* get_camera() { return camera; }

    void set_irradiance_texture(Texture* texture);
    Texture* get_irradiance_texture() { return irradiance_texture; }

};
