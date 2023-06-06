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

    enum eType {
        BUFFER,
        SAMPLER,
        TEXTURE_VIEW
    };

    Uniform();

    std::variant<WGPUBuffer, WGPUSampler, WGPUTextureView> data;

    eType type = BUFFER;

    uint32_t binding = 0;
    uint32_t visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;

    wgpu::BufferBindingType buffer_binding_type = wgpu::BufferBindingType::Uniform;
    uint64_t buffer_size;

    wgpu::TextureBindingLayout texture_binding_layout;
    wgpu::StorageTextureBindingLayout storage_texture_binding_layout;

    bool is_storage_texture = false;

    wgpu::BindGroupLayoutEntry get_bind_group_layout_entry() const;
    wgpu::BindGroupEntry       get_bind_group_entry() const;
};

struct WebGPUContext {

    wgpu::Instance            instance = nullptr;
    wgpu::Surface             surface = nullptr;
    wgpu::Device              device = nullptr;
    wgpu::Queue               device_queue = nullptr;

    wgpu::SwapChain           screen_swapchain = nullptr;
    int                       screen_width = 0;
    int                       screen_height = 0;

    static wgpu::TextureFormat swapchain_format;
    static wgpu::TextureFormat xr_swapchain_format;

    bool                      is_initialized = false;

    std::unique_ptr<wgpu::ErrorCallback> error_callback;

    GLFWwindow* window = nullptr;

    int initialize(GLFWwindow* window, bool create_screen_swapchain);

    void                     create_instance();

    wgpu::ShaderModule       create_shader_module(char const* code);

    wgpu::Buffer             create_buffer(uint64_t size, int usage, const void* data);
    wgpu::Texture            create_texture(wgpu::TextureDimension dimension, wgpu::TextureFormat format, wgpu::Extent3D size, int usage, uint32_t mipmaps);
    wgpu::TextureView        create_texture_view(wgpu::Texture texture, wgpu::TextureViewDimension dimension, wgpu::TextureFormat format);

    wgpu::BindGroupLayout    create_bind_group_layout(const std::vector<Uniform*>& uniforms);
    wgpu::BindGroup          create_bind_group(const std::vector<Uniform*>& uniforms, wgpu::BindGroupLayout bind_group_layout);
    wgpu::PipelineLayout     create_pipeline_layout(const std::vector<wgpu::BindGroupLayout>& bind_group_layouts);

    wgpu::RenderPipeline     create_render_pipeline(const std::vector<wgpu::VertexBufferLayout>& vertex_attributes, wgpu::ColorTargetState color_target, wgpu::ShaderModule render_shader_module, wgpu::PipelineLayout pipeline_layout);
    wgpu::ComputePipeline    create_compute_pipeline(wgpu::ShaderModule compute_shader_module, wgpu::PipelineLayout pipeline_layout);

    wgpu::VertexBufferLayout create_vertex_buffer_layout(const std::vector<wgpu::VertexAttribute>& vertex_attributes, uint64_t stride, wgpu::VertexStepMode step_mode);

    wgpu::Surface get_surface(GLFWwindow* window);

    void printErrors();

};