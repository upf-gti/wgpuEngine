#pragma once

#include "utils.h"
#include "openxr_context.h"
#include "graphics/webgpu_context.h"

class Renderer {

#ifdef USE_XR
    OpenXRContext xr_context;
#endif

    WebGPUContext webgpu_context;

    wgpu::RenderPipeline      render_pipeline;
    wgpu::ShaderModule        render_shader_module;
    wgpu::PipelineLayout      render_pipeline_layout;
    wgpu::BindGroupLayout     render_bind_group_layout;
    wgpu::BindGroup           render_bind_group;

    wgpu::ComputePipeline     compute_pipeline;
    wgpu::ShaderModule        compute_shader_module;
    wgpu::PipelineLayout      compute_pipeline_layout;
    wgpu::BindGroupLayout     compute_bind_group_layout;
    wgpu::BindGroup           compute_bind_group;

    wgpu::Texture             compute_texture;
    Uniform                   uniform_compute_texture;

    Uniform                   uniform_viewprojection;

#if defined(USE_XR) && defined(USE_MIRROR_WINDOW)
    wgpu::RenderPipeline      mirror_pipeline;
    wgpu::PipelineLayout      mirror_pipeline_layout;
    wgpu::BindGroupLayout     mirror_bind_group_layout;
    wgpu::BindGroup           mirror_bind_group;
    wgpu::ShaderModule        mirror_shader_module;

    wgpu::Buffer              vertex_buffer;
    Uniform                   uniform_left_eye_view;
#endif

public:
    // Methods =========================
    int initialize(GLFWwindow* window);
    void clean();

    void render();

private:

    uint32_t render_width = 0;
    uint32_t render_height = 0;

    void render(wgpu::TextureView swapchain_view, const glm::mat4x4& view_projection);
    void compute();

    void initRenderPipeline();
    void initComputePipeline();

#if defined(USE_XR) && defined(USE_MIRROR_WINDOW)
    void renderMirror();
    void initMirrorPipeline();
#endif
};
