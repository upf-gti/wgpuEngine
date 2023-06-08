#pragma once

#include "includes.h"

// Already includes dawnxr/webgpu
#include "uniform.h" 

#include "GLFW/glfw3.h"

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
    int                     render_width = 0;
    int                     screen_height = 0;

    bool                    is_initialized = false;

    GLFWwindow*             window = nullptr;

    static WGPUTextureFormat    swapchain_format;
    static WGPUTextureFormat    xr_swapchain_format;

    int                    initialize(GLFWwindow* window, bool create_screen_swapchain);
    void                   destroy();

    void                   create_instance();

    WGPUInstance           get_instance();

    WGPUShaderModule       create_shader_module(char const* code);

    WGPUBuffer             create_buffer(uint64_t size, int usage, const void* data);
    WGPUTexture            create_texture(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, int usage, uint32_t mipmaps);
    WGPUTextureView        create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format);

    WGPUBindGroupLayout    create_bind_group_layout(const std::vector<Uniform*>& uniforms);
    WGPUBindGroup          create_bind_group(const std::vector<Uniform*>& uniforms, WGPUBindGroupLayout bind_group_layout);
    WGPUPipelineLayout     create_pipeline_layout(const std::vector<WGPUBindGroupLayout>& bind_group_layouts);

    WGPURenderPipeline     create_render_pipeline(const std::vector<WGPUVertexBufferLayout>& vertex_attributes, WGPUColorTargetState color_target, WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout);
    WGPUComputePipeline    create_compute_pipeline(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout);

    WGPUVertexBufferLayout create_vertex_buffer_layout(const std::vector<WGPUVertexAttribute>& vertex_attributes, uint64_t stride, WGPUVertexStepMode step_mode);

    void printErrors();

};