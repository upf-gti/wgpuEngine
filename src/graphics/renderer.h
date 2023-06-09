#pragma once

#include "utils.h"

#include "framework/mesh.h"

#include "graphics/shader.h"
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
    Shader*                 render_shader = nullptr;

    WGPUPipelineLayout      render_pipeline_layout = nullptr;
    WGPUBindGroupLayout     render_bind_group_layout = nullptr;

    WGPUBindGroup           render_bind_group_left_eye = nullptr;
    WGPUBindGroup           render_bind_group_right_eye = nullptr;

    WGPUComputePipeline     compute_pipeline = nullptr;
    Shader*                 compute_shader = nullptr;

    WGPUPipelineLayout      compute_pipeline_layout = nullptr;

    WGPUBindGroupLayout     compute_textures_bind_group_layout = nullptr;
    WGPUBindGroup           compute_textures_bind_group = nullptr;

    WGPUBindGroupLayout     compute_data_bind_group_layout = nullptr;
    WGPUBindGroup           compute_data_bind_group = nullptr;

    WGPUTexture             left_eye_texture = nullptr;
    WGPUTexture             right_eye_texture = nullptr;

    // Uniforms

    struct sComputeData {
        glm::mat4x4 inv_view_projection_left_eye;
        glm::mat4x4 inv_view_projection_right_eye;

        glm::vec3 left_eye_pos;
        float render_height = 0.0f;
        glm::vec3 right_eye_pos;
        float render_width = 0.0f;

        float time = 0.0f;
        float dummy0 = 0.0f;
        float dummy1 = 0.0f;
        float dummy2 = 0.0f;
    };

    sComputeData            compute_data;

    Uniform                 u_compute_buffer_data;
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
    Shader*                 mirror_shader;

    Uniform                 uniform_left_eye_view;
#endif

    uint32_t render_width   = 0;
    uint32_t render_height  = 0;

    bool is_openxr_available    = false;
    bool use_mirror_screen      = false;

    void render(WGPUTextureView swapchain_view, WGPUBindGroup bind_group);
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

    Renderer();

    int initialize(GLFWwindow* window, bool use_mirror_screen);
    void clean();
    
    void update(double delta_time);
    void render();

    bool get_openxr_available();
};
