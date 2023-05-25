#pragma once

#include "utils.h"
#include "openxr_context.h"
#include "graphics/webgpu_context.h"

class Renderer {

#ifdef USE_XR
    OpenXRContext xr_context;
#endif

    WebGPUContext webgpu_context;

    wgpu::ShaderModule        shader_module;
    wgpu::RenderPipeline      render_pipeline;
    wgpu::PipelineLayout      render_pipeline_layout;
    wgpu::BindGroupLayout     render_bind_group_layout;
    wgpu::BindGroup           render_bind_group;

#if defined(USE_XR) && defined(USE_MIRROR_WINDOW)
    wgpu::SwapChain           mirror_swapchain;
    wgpu::RenderPipeline      mirror_render_pipeline;
    wgpu::PipelineLayout      mirror_render_pipeline_layout;
    wgpu::ShaderModule        mirror_shader_module;
#endif

    Uniform                   uniform_viewprojection;

public:
    // Methods =========================
    int initialize(GLFWwindow* window);
    void clean();

    void render();
    void renderXr(int swapchain_index);
    void renderMirror();

private:

    void initPipeline();
    void initMirrorPipeline();
};
