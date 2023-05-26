#pragma once

#include "includes.h"

#include <dawnxr/dawnxr.h>
#include <dawn/native/DawnNative.h>
#include "GLFW/glfw3.h"

#include <variant>
#include <functional>

class OpenXRContext;

struct Uniform {
    std::variant<wgpu::Buffer, wgpu::Sampler, wgpu::TextureView> data;

    uint32_t binding = 0;
    wgpu::ShaderStage visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;

    wgpu::BufferBindingType buffer_binding_type = wgpu::BufferBindingType::Uniform;

    wgpu::TextureBindingLayout texture_binding_layout = {
        .sampleType = wgpu::TextureSampleType::Float,
        .viewDimension = wgpu::TextureViewDimension::e2D
    };

    wgpu::StorageTextureBindingLayout storage_texture_binding_layout = {
        .access = wgpu::StorageTextureAccess::WriteOnly,
        .format = wgpu::TextureFormat::RGBA8Unorm,
        .viewDimension = wgpu::TextureViewDimension::e2D
    };

    bool is_storage_texture = false;

    wgpu::BindGroupLayoutEntry get_bind_group_layout_entry() const;
    wgpu::BindGroupEntry       get_bind_group_entry() const;
};

struct WebGPUContext {

    dawn::native::Instance*   instance;
    wgpu::Surface             surface;
    wgpu::Device              device;
    wgpu::Queue               device_queue;

#if !defined(USE_XR) || (defined(USE_XR) && defined(USE_MIRROR_WINDOW))
    wgpu::SwapChain           screen_swapchain;
    int                       screen_width = 0;
    int                       screen_height = 0;
#endif

    wgpu::TextureFormat       swapchain_format       = wgpu::TextureFormat::BGRA8Unorm;
    wgpu::TextureFormat       xr_swapchain_format    = wgpu::TextureFormat::BGRA8UnormSrgb;

    bool                      is_initialized = false;

    GLFWwindow* window = nullptr;

    int initialize(GLFWwindow* window);

    void                     create_instance();

    wgpu::ShaderModule       create_shader_module(char const* code);

    wgpu::Buffer             create_buffer(uint64_t size, wgpu::BufferUsage usage, const void* data);
    wgpu::Texture            create_texture(wgpu::TextureDimension dimension, wgpu::TextureFormat format, wgpu::Extent3D size, wgpu::TextureUsage usage, uint32_t mipmaps);
    wgpu::TextureView        create_texture_view(wgpu::Texture texture, wgpu::TextureViewDimension dimension, wgpu::TextureFormat format);

    wgpu::BindGroupLayout    create_bind_group_layout(const std::vector<Uniform>& uniforms);
    wgpu::BindGroup          create_bind_group(const std::vector<Uniform>& uniforms, wgpu::BindGroupLayout bind_group_layout);
    wgpu::PipelineLayout     create_pipeline_layout(const std::vector<wgpu::BindGroupLayout>& bind_group_layouts);

    wgpu::RenderPipeline     create_render_pipeline(const std::vector<wgpu::VertexBufferLayout>& vertex_attributes, wgpu::ColorTargetState color_target, wgpu::ShaderModule render_shader_module, wgpu::PipelineLayout pipeline_layout);
    wgpu::ComputePipeline    create_compute_pipeline(wgpu::ShaderModule compute_shader_module, wgpu::PipelineLayout pipeline_layout);

    wgpu::VertexBufferLayout create_vertex_buffer_layout(const std::vector<wgpu::VertexAttribute>& vertex_attributes, uint64_t stride, wgpu::VertexStepMode step_mode);

    wgpu::Surface get_surface(GLFWwindow* window);

};