#pragma once

#include "core/managers/render/render_methods/render_method.h"

#include "graphics/texture.h"
#include "graphics/uniform.h"

class Pipeline;
class Shader;
class Light3D;

#define ENVIRONMENT_RESOLUTION 1024
#define MAX_LIGHTS 32u
#define SHADOW_MAP_SIZE 1024

class RenderForward : public RenderMethod {
public:
    Error initialize() override;
    Error finalize() override;

    void render_scene() override;

private:

    Texture depth_buffers[EYE_COUNT] = {};
    WGPUTextureView depth_buffers_views[EYE_COUNT] = {};

    void init_depth_buffers();

    // Needed for msaa
    Texture multisample_textures[EYE_COUNT] = {};
    WGPUTextureView multisample_textures_views[EYE_COUNT] = {};

    void init_multisample_textures();

    // Lighting
    void init_lighting_bind_group();

    Pipeline panorama_to_cubemap_pipeline;
    Pipeline prefiltered_env_pipeline;
    Pipeline brdf_lut_pipeline;

    Shader* panorama_to_cubemap_shader;
    Shader* prefiltered_env_shader;
    Shader* brdf_lut_shader;

    Texture brdf_lut_texture;

    WGPUBindGroup lighting_bind_group;

    Texture* irradiance_texture;
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

    void generate_brdf_lut_texture();
    void generate_prefiltered_env_texture(Texture* prefiltered_env_texture, Texture* hdr_texture);
};
