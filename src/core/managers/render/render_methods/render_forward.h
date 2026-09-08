#pragma once

#include "core/managers/render/render_methods/render_method.h"

#include "graphics/pipeline.h"
#include "graphics/surface.h"
#include "graphics/texture.h"
#include "graphics/uniform.h"
#include "graphics/uniforms_structs.h"

#include <vector>

class Shader;
class Light3D;
class Mesh;
class GSNode;

#define ENVIRONMENT_RESOLUTION 1024
#define MAX_LIGHTS 32u
#define SHADOW_MAP_SIZE 1024

class RenderForward : public RenderMethod {
    friend class RenderManager;

    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_pre_opaque_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_post_opaque_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_pre_transparent_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_post_transparent_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_pre_2d_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_post_2d_pass = nullptr;

    void* custom_pass_user_data = nullptr;
    void set_custom_pass_user_data(void* user_data);

public:
    void render_scene(std::vector<std::vector<sRenderData>>& render_lists, const Camera3D* camera, const sEnvironmentData& environment_data, WGPUTextureView framebuffer_view, eEYE eye_idx = EYE_LEFT, const char* label = nullptr);

    void resize_swapchain() override;

    void set_msaa_count(uint8_t msaa_count);

private:
    RenderForward() = default;
    ~RenderForward() = default;

    Error initialize() override;
    Error finalize() override;

    void render_opaque(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_transparent(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_splats(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_2D(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group);

    void render_camera(const std::vector<std::vector<sRenderData>>& render_lists, WGPUTextureView framebuffer_view, WGPUTextureView multisample_framebufer_view, WGPUTextureView depth_view, bool render_transparents, const char* pass_name);

    void render_render_list(WGPURenderPassEncoder render_pass, const std::vector<sRenderData>& render_list, int list_index, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);

    void render_sky(WGPURenderPassEncoder render_pass, WGPUBindGroup camera_bind_group);

    void fill_instances_data(std::vector<std::vector<sRenderData>>& render_lists);

    sInstanceData render_instances_data;
    sInstanceData shadow_instances_data;

    Camera3D* camera_3d = nullptr;
    Camera3D* camera_2d = nullptr;

    uint8_t msaa_count = 1;

    void init_depth_buffers();

    Texture depth_buffers[EYE_COUNT] = {};
    WGPUTextureView depth_buffers_views[EYE_COUNT] = {};

    // Needed for msaa
    Texture multisample_framebuffers[EYE_COUNT] = {};
    WGPUTextureView multisample_framebuffers_views[EYE_COUNT] = {};

    void init_multisample_textures();

    struct sCameraData {
        glm::mat4x4 view_projection;
        glm::mat4x4 view;
        glm::mat4x4 projection;

        glm::vec3 eye = {};
        float exposure = 1.0f;

        glm::vec3 right_controller_position = {};
        float ibl_intensity = 1.0f;

        glm::vec2 screen_size;
        glm::vec2 dummy;
    };

    sCameraData camera_data;
    sCameraData camera_2d_data;

    Uniform camera_uniform;
    Uniform camera_2d_uniform;
    Uniform shadow_camera_uniform;

    uint32_t camera_buffer_stride = 0;

    WGPUBindGroup render_camera_bind_group = nullptr;
    WGPUBindGroup shadow_camera_bind_group = nullptr;
    WGPUBindGroup compute_camera_bind_group = nullptr;
    WGPUBindGroup render_camera_bind_group_2d = nullptr;

    void init_camera_bind_group();

    // Lighting
    void init_lighting_bind_group();

    Pipeline panorama_to_cubemap_pipeline;
    Pipeline prefiltered_env_pipeline;
    Pipeline brdf_lut_pipeline;

    Shader* panorama_to_cubemap_shader = nullptr;
    Shader* prefiltered_env_shader = nullptr;
    Shader* brdf_lut_shader = nullptr;

    Texture brdf_lut_texture;

    WGPUBindGroup lighting_bind_group;

    Texture* last_irradiance_texture_panorama = nullptr;
    Texture* irradiance_texture_cubemap = nullptr;
    Uniform irradiance_texture_uniform;
    Uniform brdf_lut_uniform;
    Uniform ibl_sampler_uniform;

    sLightUniformData lights_uniform_data[MAX_LIGHTS];
    int num_lights = 0;

    Uniform lights_buffer;
    Uniform num_lights_buffer;
    Uniform shadow_maps_array;
    Uniform shadow_sampler;
    WGPUTexture shadow_array_texture;

    uint32_t shadow_uniform_buffer_size = MAX_LIGHTS;
    std::vector<Light3D*> lights_with_shadow;

    Material* shadow_material;

    Pipeline gs_render_pipeline;
    Shader* gs_render_shader = nullptr;

    void create_sky_bind_group();

    std::vector<Uniform*> sky_uniforms;
    WGPUBindGroup sky_bind_group = nullptr;
    Mesh* skybox_mesh = nullptr;
    Surface* skybox_surface = nullptr;
    Material* skybox_material = nullptr;

    void generate_brdf_lut_texture();
    void generate_prefiltered_env_texture(Texture* prefiltered_env_texture, Texture* hdr_texture);

    // Gaussian Splatting scenes to render
    std::vector<GSNode*> gs_scenes_list;

    // Mirror
    Pipeline mirror_pipeline;
    Shader* mirror_shader = nullptr;
    WGPUBindGroup custom_mirror_fbo_bind_group = nullptr;

    Uniform linear_sampler_uniform;

    Surface quad_surface;

    std::vector<Uniform> swapchain_uniforms;
    std::vector<WGPUBindGroup> swapchain_bind_groups;
    void init_mirror_pipeline();
    void render_mirror(WGPUTextureView screen_surface_texture_view, WGPUBindGroup displayed_fbo_bind_group);

    void update_lights();
};
