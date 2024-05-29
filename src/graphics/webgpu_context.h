#pragma once

#include "includes.h"

#include "uniform.h"

#include <unordered_map>

class Shader;
class Pipeline;
class Texture;
struct GLFWwindow;

#define ENVIRONMENT_RESOLUTION 1024

struct WebGPUContext {

    WGPUInstance            instance = nullptr;

    WGPUSurface             surface = nullptr;
    WGPUDevice              device = nullptr;
    WGPUQueue               device_queue = nullptr;

    WGPUSwapChain           screen_swapchain = nullptr;

    // For desktop window
    uint32_t                screen_width = 0;
    uint32_t                screen_height = 0;

    // Internal render size, may come from openxr
    uint32_t                render_width = 0;
    uint32_t                render_height = 0;

    struct sMipmapPipeline {
        Pipeline* mipmap_pipeline;
        Shader* mipmap_shader;
    };

    sMipmapPipeline cubemap_mipmap_pipeline;

    std::unordered_map<WGPUTextureFormat, sMipmapPipeline> mipmap_pipelines;

    Pipeline*               panorama_to_cubemap_pipeline;
    Shader*                 panorama_to_cubemap_shader;

    Pipeline*               prefiltered_env_pipeline;
    Shader*                 prefiltered_env_shader;

    Pipeline*               brdf_lut_pipeline;
    Shader*                 brdf_lut_shader;
    Texture*                brdf_lut_texture = nullptr;

    WGPURequiredLimits      required_limits;

    bool                    is_initialized = false;

    GLFWwindow*             window = nullptr;

    static WGPUTextureFormat    swapchain_format;
    static WGPUTextureFormat    xr_swapchain_format;

    int                    initialize(WGPURequestAdapterOptions adapter_opts, WGPURequiredLimits required_limits, GLFWwindow* window, bool create_screen_swapchain);
    void                   destroy();

    void                   create_instance();

    WGPUInstance           get_instance();

    void                   create_swapchain(int width, int height);

    WGPUShaderModule       create_shader_module(char const* code);

    WGPUBuffer             create_buffer(uint64_t size, int usage, const void* data, const char* label = nullptr);
    WGPUTexture            create_texture(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, uint8_t sample_count);
    WGPUTextureView        create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect = WGPUTextureAspect_All, uint32_t base_mip_level = 0, uint32_t mip_level_count = 1, uint32_t base_array_layer = 0, uint32_t array_layer_count = 1, const char* label = "");
    
                           // By now wrapU = wrapV = wrapW
    WGPUSampler            create_sampler(WGPUAddressMode wrap_u = WGPUAddressMode_ClampToEdge, WGPUAddressMode wrap_v = WGPUAddressMode_ClampToEdge, WGPUAddressMode wrap_w = WGPUAddressMode_ClampToEdge, WGPUFilterMode mag_filter = WGPUFilterMode_Linear, WGPUFilterMode min_filter = WGPUFilterMode_Linear, WGPUMipmapFilterMode mipmap_filter = WGPUMipmapFilterMode_Linear, float lod_max_clamp = 1.0f, uint16_t max_anisotropy = 1);
    void                   create_texture_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, WGPUTextureViewDimension view_dimension = WGPUTextureViewDimension_2D, WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm, WGPUOrigin3D origin = {0, 0, 0}, WGPUCommandEncoder custom_command_encoder = nullptr);
    void                   create_cubemap_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, WGPUTextureViewDimension view_dimension = WGPUTextureViewDimension_2D, WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm, WGPUOrigin3D origin = {0, 0, 0}, WGPUCommandEncoder custom_command_encoder = nullptr);
    void                   upload_texture(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level, WGPUTextureFormat format, const void* data, WGPUOrigin3D origin = { 0, 0, 0 });

    WGPUBindGroupLayout    create_bind_group_layout(const std::vector<WGPUBindGroupLayoutEntry>& entries);
    WGPUBindGroup          create_bind_group(const std::vector<Uniform*>& uniforms, WGPUBindGroupLayout bind_group_layout);
    WGPUBindGroup          create_bind_group(const std::vector<Uniform*>& uniforms, Shader* shader, uint16_t bind_group);
    WGPUPipelineLayout     create_pipeline_layout(const std::vector<WGPUBindGroupLayout>& bind_group_layouts);

    void                   copy_texture_to_texture(WGPUTexture texture_src, WGPUTexture texture_dst, uint32_t src_mipmap_level, uint32_t dst_mipmap_level, const WGPUExtent3D& copy_size, WGPUCommandEncoder custom_command_encoder = nullptr);

    WGPURenderPipeline     create_render_pipeline(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes,
                                                  WGPUColorTargetState color_target, bool use_depth = true, bool depth_read = true, bool depth_write = true, WGPUCullMode cull_mode = WGPUCullMode_None, WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList, uint8_t sample_count = 1);
    WGPUComputePipeline    create_compute_pipeline(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout);

    WGPUVertexBufferLayout create_vertex_buffer_layout(const std::vector<WGPUVertexAttribute>& vertex_attributes, uint64_t stride, WGPUVertexStepMode step_mode);

    WGPUQuerySet           create_query_set(uint8_t maximum_query_sets);

    void                   generate_brdf_lut_texture();
    void                   generate_prefiltered_env_texture(Texture* prefiltered_env_texture, Texture* hdr_texture);

    void                   update_buffer(WGPUBuffer buffer, uint64_t buffer_offset, void const* data, uint64_t size);
    void                   update_texture(WGPUTexture buffer, void const* data, uint64_t size);

    void*                  read_buffer(WGPUBuffer buffer, size_t size);

    sMipmapPipeline        get_mipmap_pipeline(WGPUTextureFormat texture_format);

    void process_events();

};
