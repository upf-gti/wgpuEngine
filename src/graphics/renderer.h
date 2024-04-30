#pragma once

#include "includes.h"
#include "graphics/uniforms_structs.h"
#include "graphics/uniform.h"

#include "glm/mat4x4.hpp"

#define MAX_LIGHTS 8

class Camera;
class Texture;
class Surface;
class Light3D;
class RenderdocCapture;
class RendererStorage;
class MeshInstance;
struct GLFWwindow;
struct WebGPUContext;
struct OpenXRContext;
struct sLightUniformData;

class Renderer {

protected:
#ifdef XR_SUPPORT
    OpenXRContext*  xr_context;
#endif

    WebGPUContext*  webgpu_context;

    Camera* camera = nullptr;
    Camera* camera_2d = nullptr;

    Texture* irradiance_texture = nullptr;

    Texture*         eye_depth_textures;
    WGPUTextureView eye_depth_texture_view[EYE_COUNT] = {};

    uint8_t msaa_count = 1;
    Texture* multisample_textures;
    WGPUTextureView multisample_textures_views[EYE_COUNT] = {};

    RendererStorage* renderer_storage;

#ifndef __EMSCRIPTEN__
    RenderdocCapture* renderdoc_capture;
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
    };

    struct sRenderListData {
        MeshInstance* mesh_instance;
        glm::mat4x4 global_matrix;
    };

    struct sRenderData {
        Surface* surface;
        uint32_t repeat;
        glm::mat4x4 global_matrix;
        glm::mat4x4 rotation_matrix;
        MeshInstance* mesh_instance_ref;
    };

    enum eRenderListType {
        RENDER_LIST_OPAQUE,
        RENDER_LIST_TRANSPARENT,
        RENDER_LIST_2D,
        RENDER_LIST_2D_TRANSPARENT,
        RENDER_LIST_SIZE
    };

    void render_render_list(int list_index, WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera, uint32_t camera_buffer_stride = 0);

    std::vector<sUniformData> instance_data[RENDER_LIST_SIZE];
    Uniform	instance_data_uniform[RENDER_LIST_SIZE];

    std::vector<sUIData> instance_ui_data;
    Uniform	instance_ui_data_uniform;

    // Entities to be rendered this frame
    std::vector<sRenderListData> render_entity_list;

    std::vector<sRenderData> render_list[RENDER_LIST_SIZE];

    // Bind group per shader
    WGPUBindGroup bind_groups[RENDER_LIST_SIZE] = {};

    // Bind group for lighting

    WGPUBindGroup lighting_bind_group;

    // Indirect lighting

    Uniform irradiance_texture_uniform;
    Uniform brdf_lut_uniform;
    Uniform ibl_sampler_uniform;

    // Direct lighting

    sLightUniformData lights_uniform_data[MAX_LIGHTS];
    int num_lights = 0;

    Uniform lights_buffer;
    Uniform num_lights_buffer;

public:

    // Singleton
    static Renderer* instance;

    Renderer();
    virtual ~Renderer();

    virtual int initialize(GLFWwindow* window, bool use_mirror_screen = false);
    virtual void clean();
    
    virtual void update(float delta_time) = 0;
    virtual void render() = 0;

    void init_lighting_bind_group();
    WGPUBindGroup get_lighting_bind_group() { return lighting_bind_group; }

    void init_depth_buffers();
    void init_multisample_textures();

    void set_msaa_count(uint8_t msaa_count);
    uint8_t get_msaa_count();

    void prepare_instancing();
    void render_opaque(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera, uint32_t camera_buffer_stride = 0);
    void render_transparent(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera, uint32_t camera_buffer_stride = 0);
    void render_2D(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera);

    bool get_openxr_available() { return is_openxr_available; }
    bool get_use_mirror_screen() { return use_mirror_screen; }

    glm::vec4 get_clear_color() { return clear_color; }

#ifdef XR_SUPPORT
    OpenXRContext* get_openxr_context();
#endif
    WebGPUContext* get_webgpu_context();

    void set_required_limits(const WGPURequiredLimits& required_limits) { this->required_limits = required_limits; }

    void add_renderable(MeshInstance* mesh_instance, glm::mat4x4 global_matrix);
    void clear_renderables();

    void update_lights();
    void add_light(Light3D* new_light);

    virtual void resize_window(int width, int height);

    GLFWwindow* get_glfw_window();

    void set_irradiance_texture(Texture* texture);
    Texture* get_irradiance_texture() { return irradiance_texture; }

    Camera* get_camera() { return camera; }
    glm::vec3 get_camera_eye();
};
