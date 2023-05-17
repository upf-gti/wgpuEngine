#pragma once

#include <dawnxr/dawnxr.h>
#include <dawn/native/DawnNative.h>
#include "GLFW/glfw3.h"

class OpenXRContext;

struct WebGPUContext {

    dawn::native::Instance*   dawnInstance;
    //wgpu::Instance            wgpuInstance;
    //wgpu::Adapter             adapter;
    //wgpu::Surface             surface;
    wgpu::Device              device;
    wgpu::Queue               device_queue;
    wgpu::CommandEncoder      device_command_encoder;
    wgpu::TextureFormat       swapchain_format;
    //wgpu::SwapChain           swapchain;
    wgpu::ShaderModule        shader_module;
    wgpu::RenderPipeline      render_pipeline;
    wgpu::PipelineLayout      render_pipeline_layout;

    wgpu::SwapChain           mirror_swapchain;

    bool                      is_initialized = false;
    wgpu::TextureView         current_texture_view;

    GLFWwindow* window = nullptr;

    dawn::native::AdapterDiscoveryOptionsBase** options;

    std::vector<dawnxr::SwapchainImageDawn> images;

    int initialize(OpenXRContext* xr_context, GLFWwindow* window);
    void config_render_pipeline();
    wgpu::Surface get_surface(GLFWwindow* window);

};