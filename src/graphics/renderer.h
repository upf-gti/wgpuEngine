#pragma once

#include "includes.h"
#include "graphics/uniforms_structs.h"
#include "graphics/uniform.h"
#include "framework/math/frustum_cull.h"
#include "graphics/surface.h"
#include "graphics/pipeline.h"

#include "glm/mat4x4.hpp"

#include "backends/imgui_impl_wgpu.h"

#include <map>
#include <string>

#define MAX_LIGHTS 8

class Camera;
class Texture;
class Surface;
class Light3D;
class RenderdocCapture;
class RendererStorage;
class MeshInstance;
class MeshInstance3D;
struct GLFWwindow;
struct WebGPUContext;
struct OpenXRContext;
struct sLightUniformData;

class Renderer {

protected:
#ifdef XR_SUPPORT
    OpenXRContext*  xr_context;
#endif

    WebGPUContext*  webgpu_context;

    std::function<void(void*, WGPURenderPassEncoder, uint32_t)> custom_pre_opaque_pass = nullptr;
    std::function<void(void*, WGPURenderPassEncoder, uint32_t)> custom_post_opaque_pass = nullptr;

    std::function<void(void*, WGPURenderPassEncoder, uint32_t)> custom_pre_transparent_pass = nullptr;
    std::function<void(void*, WGPURenderPassEncoder, uint32_t)> custom_post_transparent_pass = nullptr;

    std::function<void(void*, WGPURenderPassEncoder, uint32_t)> custom_pre_2d_pass = nullptr;
    std::function<void(void*, WGPURenderPassEncoder, uint32_t)> custom_post_2d_pass = nullptr;

    void* custom_pass_user_data = nullptr;

    WGPUCommandEncoder global_command_encoder;

    Camera* camera = nullptr;
    Camera* camera_2d = nullptr;

    // Render meshes with material color
    WGPUBindGroup render_camera_bind_group = nullptr;
    WGPUBindGroup compute_camera_bind_group = nullptr;
    WGPUBindGroup render_camera_bind_group_2d = nullptr;

    Uniform camera_uniform;
    Uniform camera_2d_uniform;

    uint32_t xr_camera_buffer_stride = 0;

    Texture* irradiance_texture = nullptr;

    Texture*        eye_depth_textures;
    WGPUTextureView eye_depth_texture_view[EYE_COUNT] = {};

    uint8_t msaa_count = 1;
    Texture* multisample_textures;
    WGPUTextureView multisample_textures_views[EYE_COUNT] = {};

    RendererStorage* renderer_storage;

#ifndef __EMSCRIPTEN__
    RenderdocCapture* renderdoc_capture;
#endif

    Frustum frustum_cull;
    MeshInstance3D* selected_mesh_aabb = nullptr;

    bool is_openxr_available    = false;
    bool use_mirror_screen      = false;

    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

    // Required device limits
    WGPURequiredLimits required_limits = {};

    struct sUniformData {
        glm::mat4x4 model;
    };

    struct sRenderListData {
        MeshInstance* mesh_instance;
        glm::mat4x4 global_matrix;
    };

    struct sRenderData {
        Surface* surface;
        uint32_t repeat;
        glm::mat4x4 global_matrix;
        MeshInstance* mesh_instance_ref;
    };

    enum eRenderListType {
        RENDER_LIST_OPAQUE,
        RENDER_LIST_TRANSPARENT,
        RENDER_LIST_2D,
        RENDER_LIST_2D_TRANSPARENT,
        RENDER_LIST_SIZE
    };

    struct sCameraData {
        glm::mat4x4 view_projection;
        glm::mat4x4 view;
        glm::mat4x4 projection;

        glm::vec3 eye = {};
        float exposure = 1.0f;

        glm::vec3 right_controller_position = {};
        float ibl_intensity = 1.0f;

        glm::vec2 screen_size;
        glm::vec2 dummy;
    };

    sCameraData camera_data;
    sCameraData camera_2d_data;

    void render_render_list(int list_index, WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group, uint32_t camera_buffer_stride = 0);

    void init_camera_bind_group();

    std::vector<float> get_timestamps();

    std::vector<sUniformData> instance_data[RENDER_LIST_SIZE];
    Uniform	instance_data_uniform[RENDER_LIST_SIZE];

    std::vector<sUIData> instance_ui_data;
    Uniform	instance_ui_data_uniform;

    // Entities to be rendered this frame
    std::vector<sRenderListData> render_entity_list;

    std::vector<sRenderData> render_list[RENDER_LIST_SIZE];

    // Bind group per shader
    WGPUBindGroup bind_groups[RENDER_LIST_SIZE] = {};

    // Bind group for lighting

    WGPUBindGroup lighting_bind_group;

    // Indirect lighting

    Uniform irradiance_texture_uniform;
    Uniform brdf_lut_uniform;
    Uniform ibl_sampler_uniform;

    // Direct lighting

    sLightUniformData lights_uniform_data[MAX_LIGHTS];
    int num_lights = 0;

    Uniform lights_buffer;
    Uniform num_lights_buffer;


    WGPUQuerySet timestamp_query_set;
    uint8_t maximum_query_sets = 16;
    uint64_t* timestamps_buffer;

    WGPUBuffer timestamp_query_buffer;
    uint8_t query_index = 0;
    std::map<uint8_t, std::string> queries_label_map;
    std::vector<float> last_frame_timestamps;

    bool frustum_camera_paused = false;
    bool debug_this_frame = false;

    void render_screen(WGPUTextureView swapchain_view);

#if defined(XR_SUPPORT)
    void render_xr();
#endif // XR_SUPPORT

    float exposure = 1.0f;
    float ibl_intensity = 1.0f;

public:

    // Singleton
    static Renderer* instance;

    Renderer();
    virtual ~Renderer();

    virtual int initialize(GLFWwindow* window, bool use_mirror_screen = false);
    virtual void clean();
    
    virtual void update(float delta_time);
    virtual void render();

    void submit_global_command_encoder();

    void set_custom_pass_user_data(void* user_data);

    void init_lighting_bind_group();
    WGPUBindGroup get_lighting_bind_group() { return lighting_bind_group; }
    WGPUBindGroup get_render_camera_bind_group() { return render_camera_bind_group; }
    WGPUBindGroup get_compute_camera_bind_group() { return compute_camera_bind_group; }

    void init_depth_buffers();
    void init_multisample_textures();
    void init_timestamp_queries();

    void set_frustum_camera_paused(bool value);
    bool get_frustum_camera_paused();

    WGPUCommandEncoder get_global_command_encoder() { return global_command_encoder; }

    void resolve_query_set(WGPUCommandEncoder encoder, uint8_t first_query);
    std::vector<float>& get_last_frame_timestamps() { return last_frame_timestamps; }

    void set_msaa_count(uint8_t msaa_count);
    uint8_t get_msaa_count();

    bool is_inside_frustum(const glm::vec3& minp, const glm::vec3& maxp) const;

    void prepare_instancing(const glm::vec3& camera_position);
    void render_opaque(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_transparent(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_2D(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group);

    bool get_openxr_available() { return is_openxr_available; }
    bool get_use_mirror_screen() { return use_mirror_screen; }

    inline void set_exposure(float new_exposure) { exposure = new_exposure; }
    inline void set_ibl_intensity(float new_intensity) { ibl_intensity = new_intensity; }

    inline void toogle_frame_debug() { debug_this_frame = true; }

    float get_exposure() { return exposure; }
    float get_ibl_intensity() { return ibl_intensity; }

    inline Uniform* get_current_camera_uniform() { return &camera_uniform; }
    glm::vec3 get_camera_eye();
    glm::vec3 get_camera_front();

    // For the XR mirror screen
#if defined(USE_MIRROR_WINDOW)
    void render_mirror(WGPUTextureView swapchain_view);
    void init_mirror_pipeline();

    Pipeline mirror_pipeline;
    Shader* mirror_shader = nullptr;

    Uniform linear_sampler_uniform;

    Surface quad_surface;

    std::vector<Uniform> swapchain_uniforms;
    std::vector<WGPUBindGroup> swapchain_bind_groups;
#endif // USE_MIRROR_WINDOW

    uint8_t timestamp(WGPUCommandEncoder encoder, const char* label = "");

    WGPUQuerySet get_query_set() { return timestamp_query_set; }
    std::map<uint8_t, std::string>& get_queries_label_map() { return queries_label_map; }

    glm::vec4 get_clear_color() { return clear_color; }

#ifdef XR_SUPPORT
    OpenXRContext* get_openxr_context();
#endif
    WebGPUContext* get_webgpu_context();

    void set_required_limits(const WGPURequiredLimits& required_limits) { this->required_limits = required_limits; }

    void add_renderable(MeshInstance* mesh_instance, const glm::mat4x4& global_matrix);
    void clear_renderables();

    void update_lights();
    void add_light(Light3D* new_light);

    virtual void resize_window(int width, int height);

    GLFWwindow* get_glfw_window();

    void set_irradiance_texture(Texture* texture);
    Texture* get_irradiance_texture() { return irradiance_texture; }

    Camera* get_camera() { return camera; }
};
