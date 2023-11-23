#pragma once

#include "includes.h"

#ifdef XR_SUPPORT
#include <dawnxr/dawnxr.h>
#endif

#include "uniform.h" 

#include "GLFW/glfw3.h"

class Shader;
class Pipeline;

struct WebGPUContext {

#ifdef XR_SUPPORT
    dawn::native::Instance* instance;
#else
    WGPUInstance            instance = nullptr;
#endif

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

    Pipeline*               mipmaps_pipeline;
    Shader*                 mipmaps_shader;

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
    WGPUTexture            create_texture(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps);
    WGPUTextureView        create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect = WGPUTextureAspect_All, uint32_t mip_level_count = 1, uint32_t base_mip_level = 0, uint32_t array_layer_count = 1, const char* label = "");
    
                           // By now wrapU = wrapV = wrapW
    WGPUSampler            create_sampler(WGPUAddressMode wrap = WGPUAddressMode_ClampToEdge, WGPUFilterMode mag_filter = WGPUFilterMode_Linear, WGPUFilterMode min_filter = WGPUFilterMode_Linear, WGPUMipmapFilterMode mipmap_filter = WGPUMipmapFilterMode_Linear, float lod_max_clamp = 1.0f, uint16_t max_anisotropy = 1);
    void                   create_texture_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, const void* data, WGPUTextureViewDimension view_dimension = WGPUTextureViewDimension_2D, WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm, WGPUOrigin3D origin = {0, 0, 0});

    WGPUBindGroupLayout    create_bind_group_layout(const std::vector<WGPUBindGroupLayoutEntry>& entries);
    WGPUBindGroup          create_bind_group(const std::vector<Uniform*>& uniforms, WGPUBindGroupLayout bind_group_layout);
    WGPUBindGroup          create_bind_group(const std::vector<Uniform*>& uniforms, Shader* shader, uint16_t bind_group);
    WGPUPipelineLayout     create_pipeline_layout(const std::vector<WGPUBindGroupLayout>& bind_group_layouts);

    WGPURenderPipeline     create_render_pipeline(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes,
                                                  WGPUColorTargetState color_target, bool uses_depth_buffer = false, WGPUCullMode cull_mode = WGPUCullMode_None, WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList);
    WGPUComputePipeline    create_compute_pipeline(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout);

    WGPUVertexBufferLayout create_vertex_buffer_layout(const std::vector<WGPUVertexAttribute>& vertex_attributes, uint64_t stride, WGPUVertexStepMode step_mode);

    void                   update_buffer(WGPUBuffer buffer, uint64_t buffer_offset, void const* data, uint64_t size);

    void print_errors();

};
