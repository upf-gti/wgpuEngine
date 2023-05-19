#pragma once

#include <dawnxr/dawnxr.h>
#include <dawn/native/DawnNative.h>
#include "GLFW/glfw3.h"

class OpenXRContext;

struct WebGPUContext {

    dawn::native::Instance*   instance;
    wgpu::Surface             surface;
    wgpu::Device              device;
    wgpu::Queue               device_queue;
    wgpu::CommandEncoder      device_command_encoder;
    wgpu::TextureFormat       swapchain_format;
    wgpu::ShaderModule        shader_module;
    wgpu::RenderPipeline      render_pipeline;
    wgpu::PipelineLayout      render_pipeline_layout;

    wgpu::SwapChain           mirror_swapchain;
    wgpu::TextureFormat       mirror_swapchain_format;
    wgpu::RenderPipeline      mirror_render_pipeline;
    wgpu::PipelineLayout      mirror_render_pipeline_layout;
    wgpu::ShaderModule        mirror_shader_module;

    bool                      is_initialized = false;

    GLFWwindow* window = nullptr;

    dawn::native::AdapterDiscoveryOptionsBase** options;

    int initialize(OpenXRContext* xr_context, GLFWwindow* window);
    void config_render_pipeline();
    void config_mirror_render_pipeline();

    wgpu::Surface get_surface(GLFWwindow* window);

};