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

#ifdef USE_MIRROR_WINDOW
    wgpu::RenderPipeline      mirror_pipeline;
    wgpu::PipelineLayout      mirror_pipeline_layout;
    wgpu::BindGroupLayout     mirror_bind_group_layout;
    wgpu::BindGroup           mirror_bind_group;
    wgpu::ShaderModule        mirror_shader_module;

    Uniform                   uniform_left_eye_view;
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
