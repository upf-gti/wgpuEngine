#pragma once

#include "utils.h"
#include "framework/mesh.h"

#include "graphics/webgpu_context.h"

#ifdef XR_SUPPORT
#include "xr/openxr_context.h"
#endif

class Renderer {

#ifdef XR_SUPPORT
    OpenXRContext           xr_context;
#endif

    WebGPUContext           webgpu_context;

    WGPURenderPipeline      render_pipeline = nullptr;
    WGPUShaderModule        render_shader_module = nullptr;

    WGPUPipelineLayout      render_pipeline_layout = nullptr;
    WGPUBindGroupLayout     render_bind_group_layout = nullptr;

    WGPUBindGroup           render_bind_group_left_eye = nullptr;
    WGPUBindGroup           render_bind_group_right_eye = nullptr;

    WGPUComputePipeline     compute_pipeline = nullptr;
    WGPUShaderModule        compute_shader_module = nullptr;
    WGPUPipelineLayout      compute_pipeline_layout = nullptr;
    WGPUBindGroupLayout     compute_bind_group_layout = nullptr;
    WGPUBindGroup           compute_bind_group = nullptr;

    WGPUTexture             left_eye_texture = nullptr;
    WGPUTexture             right_eye_texture = nullptr;

    // Uniforms
    Uniform                 u_buffer_viewprojection;
    Uniform                 u_compute_texture_left_eye;
    Uniform                 u_compute_texture_right_eye;
    Uniform                 u_render_texture_left_eye;
    Uniform                 u_render_texture_right_eye;

    Mesh                              quad_mesh;
    std::vector<WGPUVertexAttribute>  quad_vertex_attributes;
    WGPUVertexBufferLayout            quad_vertex_layout;
    WGPUBuffer                        quad_vertex_buffer = nullptr;

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)
    WGPURenderPipeline      mirror_pipeline;
    WGPUPipelineLayout      mirror_pipeline_layout;
    WGPUBindGroupLayout     mirror_bind_group_layout;
    WGPUBindGroup           mirror_bind_group;
    WGPUShaderModule        mirror_shader_module;

    Uniform                 uniform_left_eye_view;
#endif

    uint32_t render_width   = 0;
    uint32_t render_height  = 0;

    bool is_openxr_available    = false;
    bool use_mirror_screen      = false;

    void render(WGPUTextureView swapchain_view, WGPUBindGroup bind_group, const glm::mat4x4& view_projection);
    void render_screen();

#if defined(XR_SUPPORT)
    void render_xr();
#endif
    
    void compute();

    void init_render_pipeline();
    void init_compute_pipeline();

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)
    void render_mirror();
    void init_mirror_pipeline();
#endif

public:

    int initialize(GLFWwindow* window, bool use_mirror_screen);
    void clean();

    void render();

    bool get_openxr_available();
};
