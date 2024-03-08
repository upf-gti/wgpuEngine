#pragma once

#include "framework/utils/utils.h"

#include "graphics/shader.h"
#include "graphics/pipeline.h"
#include "graphics/webgpu_context.h"
#include "graphics/renderer_storage.h"
#include "graphics/surface.h"
#include "texture.h"

#ifdef XR_SUPPORT
#include "xr/openxr_context.h"
#endif

#include "framework/nodes/mesh_instance_3d.h"

#include "graphics/debug/renderdoc_capture.h"

class Camera;

class Renderer {

protected:
#ifdef XR_SUPPORT
    OpenXRContext   xr_context;
#endif

    WebGPUContext   webgpu_context;

    Camera* camera = nullptr;
    Camera* camera_2d = nullptr;

    Texture* irradiance_texture = nullptr;

    Texture         eye_depth_textures[EYE_COUNT] = {};
    WGPUTextureView eye_depth_texture_view[EYE_COUNT] = {};

    uint8_t msaa_count = 1;
    Texture multisample_textures[EYE_COUNT];
    WGPUTextureView multisample_textures_views[EYE_COUNT];

    RendererStorage renderer_storage;

#ifndef __EMSCRIPTEN__
    RenderdocCapture renderdoc_capture;
#endif

    bool is_openxr_available    = false;
    bool use_mirror_screen      = false;

    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    float z_near = 0.001f;
    float z_far = 1000.0f;

    // Required device limits
    WGPURequiredLimits required_limits = {};

    struct sUniformData {
        glm::mat4x4 model;
        glm::mat4x4 rotation;
        glm::vec4   color;
    };

    struct sRenderData {
        Surface* surface;
        uint32_t repeat;
        glm::mat4x4 global_matrix;
        glm::mat4x4 rotation_matrix;
        MeshInstance3D* entity_mesh_ref;
    };

    enum eRenderListType {
        RENDER_LIST_OPAQUE,
        RENDER_LIST_TRANSPARENT,
        RENDER_LIST_2D,
        RENDER_LIST_2D_TRANSPARENT,
        RENDER_LIST_SIZE
    };

    void render_render_list(int list_index, WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);

    std::vector<sUniformData> instance_data[RENDER_LIST_SIZE];
    Uniform	instance_data_uniform[RENDER_LIST_SIZE];

    std::vector<RendererStorage::sUIData> instance_ui_data;
    Uniform	instance_ui_data_uniform;

    // Entities to be rendered this frame
    std::vector<MeshInstance3D*> render_entity_list;

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

    void init_depth_buffers();
    void init_multisample_textures();

    void set_msaa_count(uint8_t msaa_count);
    uint8_t get_msaa_count();

    void prepare_instancing();
    void render_opaque(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);
    void render_transparent(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);
    void render_2D(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);

    bool get_openxr_available() { return is_openxr_available; }
    bool get_use_mirror_screen() { return use_mirror_screen; }

    glm::vec4 get_clear_color() { return clear_color; }

#ifdef XR_SUPPORT
    OpenXRContext* get_openxr_context() { return (is_openxr_available ? &xr_context : nullptr); }
#endif
    WebGPUContext* get_webgpu_context() { return &webgpu_context; }

    void set_required_limits(const WGPURequiredLimits& required_limits) { this->required_limits = required_limits; }

    void add_renderable(MeshInstance3D* entity_mesh);
    void clear_renderables();

    virtual void resize_window(int width, int height);

    GLFWwindow* get_glfw_window() { return webgpu_context.window; };

    void set_irradiance_texture(Texture* texture);
    Texture* get_irradiance_texture() { return irradiance_texture; }

    Camera* get_camera() { return camera; }
    glm::vec3 get_camera_eye();
};
