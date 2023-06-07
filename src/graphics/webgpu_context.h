#pragma once

#include "includes.h"

#ifdef XR_SUPPORT
#include <dawnxr/dawnxr.h>
#else
#include "webgpu/webgpu.hpp"
#endif

#ifndef __EMSCRIPTEN__
//#include <dawn/native/DawnNative.h>
#include "GLFW/glfw3.h"
#endif

#include <variant>
#include <functional>

struct Uniform {

    Uniform();

    std::variant<WGPUBuffer, WGPUSampler, WGPUTextureView> data;

    uint32_t binding = 0;
    uint32_t visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

    WGPUBufferBindingType buffer_binding_type = WGPUBufferBindingType_Uniform;
    uint64_t buffer_size;

    WGPUTextureBindingLayout texture_binding_layout;
    WGPUStorageTextureBindingLayout storage_texture_binding_layout;

    bool is_storage_texture = false;

    WGPUBindGroupLayoutEntry get_bind_group_layout_entry() const;
    WGPUBindGroupEntry       get_bind_group_entry() const;
};

struct WebGPUContext {

#ifndef __EMSCRIPTEN__
    dawn::native::Instance* instance;
#else
    WGPUInstance            instance = nullptr;
#endif

    WGPUSurface             surface = nullptr;
    WGPUDevice              device = nullptr;
    WGPUQueue               device_queue = nullptr;

    WGPUSwapChain           screen_swapchain = nullptr;
    int                     screen_width = 0;
    int                     screen_height = 0;

    static WGPUTextureFormat swapchain_format;
    static WGPUTextureFormat xr_swapchain_format;

    bool                      is_initialized = false;

    GLFWwindow* window = nullptr;

    int initialize(GLFWwindow* window, bool create_screen_swapchain);

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

    WGPUSurface get_surface(GLFWwindow* window);

    void printErrors();

};