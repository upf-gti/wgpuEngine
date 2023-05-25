#pragma once

#include <dawnxr/dawnxr.h>
#include <dawn/native/DawnNative.h>
#include "GLFW/glfw3.h"

#include <variant>

class OpenXRContext;

struct Uniform {
    std::variant<wgpu::Buffer, wgpu::Sampler, wgpu::TextureView> data;

    uint32_t binding = 0;
    wgpu::ShaderStage visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;

    wgpu::BindGroupLayoutEntry get_bind_group_layout_entry() const;
    wgpu::BindGroupEntry       get_bind_group_entry() const;
};

struct WebGPUContext {

    // Internal WebGPU
    dawn::native::Instance*   instance;
    wgpu::Surface             surface;
    wgpu::Device              device;
    wgpu::Queue               device_queue;
    wgpu::CommandEncoder      device_command_encoder;
    wgpu::TextureFormat       swapchain_format;

    // Render-related (TODO: move to Renderer class)
    wgpu::ShaderModule        shader_module;
    wgpu::RenderPipeline      render_pipeline;
    wgpu::PipelineLayout      render_pipeline_layout;
    wgpu::BindGroupLayout     render_bind_group_layout;
    wgpu::BindGroup           render_bind_group;

    wgpu::SwapChain           mirror_swapchain;
    wgpu::TextureFormat       mirror_swapchain_format;
    wgpu::RenderPipeline      mirror_render_pipeline;
    wgpu::PipelineLayout      mirror_render_pipeline_layout;
    wgpu::ShaderModule        mirror_shader_module;

    Uniform                   uniform_buffer;

    bool                      is_initialized = false;

    GLFWwindow* window = nullptr;

    dawn::native::AdapterDiscoveryOptionsBase** options;

    int initialize(OpenXRContext* xr_context, GLFWwindow* window);
    void config_render_pipeline();
    void config_mirror_render_pipeline();

    wgpu::Buffer            createBuffer(uint64_t size, wgpu::BufferUsage usage, const void* data);

    wgpu::BindGroupLayout   create_bind_group_layout(const std::vector<Uniform>& uniforms);
    wgpu::BindGroup         create_bind_group(const std::vector<Uniform>& uniforms);
    wgpu::PipelineLayout    create_pipeline_layout(const std::vector<wgpu::BindGroupLayout>& bind_group_layouts);

    wgpu::Surface get_surface(GLFWwindow* window);

};