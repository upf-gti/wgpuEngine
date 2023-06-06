#pragma once

#include "utils.h"

#ifdef XR_SUPPORT

#include "xr/openxr_context.h"

#endif

#include "graphics/webgpu_context.h"

class Renderer {

#ifdef XR_SUPPORT
    OpenXRContext xr_context;
#endif

    WebGPUContext webgpu_context;

    wgpu::RenderPipeline      render_pipeline;
    wgpu::ShaderModule        render_shader_module;

    wgpu::PipelineLayout      render_pipeline_layout;
    wgpu::BindGroupLayout     render_bind_group_layout;

    wgpu::BindGroup           render_bind_group_left_eye;
    wgpu::BindGroup           render_bind_group_right_eye;

    wgpu::ComputePipeline     compute_pipeline;
    wgpu::ShaderModule        compute_shader_module;
    wgpu::PipelineLayout      compute_pipeline_layout;
    wgpu::BindGroupLayout     compute_bind_group_layout;
    wgpu::BindGroup           compute_bind_group;

    wgpu::Texture             left_eye_texture;
    wgpu::Texture             right_eye_texture;

    // Uniforms
    Uniform                   u_buffer_viewprojection;
    Uniform                   u_compute_texture_left_eye;
    Uniform                   u_compute_texture_right_eye;
    Uniform                   u_render_texture_left_eye;
    Uniform                   u_render_texture_right_eye;

    std::vector<wgpu::VertexAttribute>  quad_vertex_attributes;
    wgpu::VertexBufferLayout            quad_vertex_layout;
    wgpu::Buffer                        quad_vertex_buffer;

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)
    wgpu::RenderPipeline      mirror_pipeline;
    wgpu::PipelineLayout      mirror_pipeline_layout;
    wgpu::BindGroupLayout     mirror_bind_group_layout;
    wgpu::BindGroup           mirror_bind_group;
    wgpu::ShaderModule        mirror_shader_module;

    Uniform                   uniform_left_eye_view;
#endif

public:
    // Methods =========================
    int initialize(GLFWwindow* window, bool use_mirror_screen);
    void clean();

    void render();

    bool isOpenXRAvailable();

private:

    uint32_t render_width = 0;
    uint32_t render_height = 0;

    bool is_openxr_available = false;
    bool use_mirror_screen = false;

    void render(wgpu::TextureView swapchain_view, wgpu::BindGroup bind_group, const glm::mat4x4& view_projection);
    void renderScreen();

#if defined(XR_SUPPORT)
    void renderXr();
#endif
    
    void compute();

    void initRenderPipeline();
    void initComputePipeline();

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)
    void renderMirror();
    void initMirrorPipeline();
#endif
};
