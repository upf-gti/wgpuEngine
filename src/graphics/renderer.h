#pragma once

#include "includes.h"
#include "graphics/uniforms_structs.h"
#include "graphics/uniform.h"
#include "framework/math/frustum_cull.h"
#include "graphics/surface.h"
#include "graphics/pipeline.h"

#include "framework/camera/camera.h"

#include "glm/mat4x4.hpp"

#include "backends/imgui_impl_wgpu.h"

#include <map>
#include <string>

#define MAX_LIGHTS 32

class Camera;
class Texture;
class Surface;
class Light3D;
class RenderdocCapture;
class RendererStorage;
class MeshInstance;
class MeshInstance3D;
class GSNode;
struct GLFWwindow;
struct WebGPUContext;
struct OpenXRContext;
struct sLightUniformData;

struct sRendererConfiguration {
    WGPURequiredLimits required_limits = {};
    std::vector<WGPUFeatureName> features;

    sRendererConfiguration() {
        required_limits.limits.maxVertexAttributes = 4;
        required_limits.limits.maxVertexBuffers = 1;
        required_limits.limits.maxBindGroups = 2;
        required_limits.limits.maxUniformBuffersPerShaderStage = 1;
        required_limits.limits.maxUniformBufferBindingSize = 65536;
        required_limits.limits.minUniformBufferOffsetAlignment = 256;
        required_limits.limits.minStorageBufferOffsetAlignment = 256;
        required_limits.limits.maxComputeInvocationsPerWorkgroup = 256;
        required_limits.limits.maxSamplersPerShaderStage = 1;
        required_limits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

#if !defined(__EMSCRIPTEN__)
        features.push_back(WGPUFeatureName_TimestampQuery);
#endif
    }
};

class Renderer {

protected:

    OpenXRContext*  xr_context;
    WebGPUContext*  webgpu_context;

    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_pre_opaque_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_post_opaque_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_pre_transparent_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_post_transparent_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_pre_2d_pass = nullptr;
    std::function<void(WGPURenderPassEncoder, WGPUBindGroup, void*, uint32_t)> custom_post_2d_pass = nullptr;

    void* custom_pass_user_data = nullptr;

    WGPUCommandEncoder global_command_encoder;

    Camera* camera_3d = nullptr;
    Camera* camera_2d = nullptr;

    // Render meshes with material color
    WGPUBindGroup render_camera_bind_group = nullptr;
    WGPUBindGroup shadow_camera_bind_group = nullptr;
    WGPUBindGroup compute_camera_bind_group = nullptr;
    WGPUBindGroup render_camera_bind_group_2d = nullptr;

    Uniform camera_uniform;
    Uniform camera_2d_uniform;
    Uniform shadow_camera_uniform;

    uint32_t camera_buffer_stride = 0;

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
    bool use_custom_mirror      = true;

    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

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
        Material* material;
    };

    enum eRenderListType {
        RENDER_LIST_OPAQUE,
        RENDER_LIST_TRANSPARENT,
        RENDER_LIST_SPLATS,
        RENDER_LIST_2D,
        RENDER_LIST_2D_TRANSPARENT,
        RENDER_LIST_COUNT
    };

    struct sInstanceData {
        std::vector<sUniformData> instances_data[RENDER_LIST_COUNT];
        Uniform	instances_data_uniforms[RENDER_LIST_COUNT];
        WGPUBindGroup instances_bind_groups[RENDER_LIST_COUNT] = {};
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

    eCameraType camera_type = CAMERA_FLYOVER;
    sCameraData camera_data;
    sCameraData camera_2d_data;

    void render_shadow_maps();

    void render_render_list(WGPURenderPassEncoder render_pass, const std::vector<sRenderData>& render_list, int list_index, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);

    void init_camera_bind_group();

    void get_timestamps();

    sInstanceData render_instances_data;
    sInstanceData shadow_instances_data;

    std::vector<sUIData> instance_ui_data;
    Uniform	instance_ui_data_uniform;

    // Entities to be rendered this frame
    std::vector<sRenderListData> render_entity_list;
    uint32_t current_render_list_size = 32;

    // Gaussian Splatting scenes to render
    std::vector<GSNode*> gs_scenes_list;

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

    // Shadows

    uint32_t shadow_uniform_buffer_size = MAX_LIGHTS;
    std::vector<Light3D*> lights_with_shadow;

    Material* shadow_material;

    Pipeline gs_render_pipeline;
    Shader* gs_render_shader = nullptr;

    WGPUQuerySet timestamp_query_set;
    uint8_t maximum_query_sets = 16;

    WGPUBuffer timestamp_query_buffer;
    uint8_t query_index = 0;
    std::map<uint8_t, std::string> queries_label_map;
    std::vector<float> last_frame_timestamps;
    bool timestamps_requested = false;

    bool frustum_camera_paused = false;
    bool debug_this_frame = false;

    float exposure = 1.0f;
    float ibl_intensity = 1.0f;

    bool initialized = false;

    std::vector<WGPUFeatureName> required_features = { };

    uint32_t frame_counter = 0;

    struct sTextureToStoreCmd {
        FILE* dst_file;
        WGPUBuffer src_buffer;
        size_t  copy_size;
        WGPUExtent3D size;
    };

    std::vector<sTextureToStoreCmd> textures_to_store_list;

public:

    // Singleton
    static Renderer* instance;

    Renderer(const sRendererConfiguration& config = {});
    virtual ~Renderer();

    virtual int pre_initialize(GLFWwindow* window, bool use_mirror_screen = false);
    virtual int initialize();
    virtual int post_initialize();
    virtual void clean();

    bool is_initialized() { return initialized; }
    
    virtual void update(float delta_time);
    virtual void render();

    void render_camera(const std::vector<std::vector<sRenderData>>& render_lists, WGPUTextureView framebuffer_view, WGPUTextureView depth_view,
        const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, bool render_transparents = true, const std::string& pass_name = "", uint32_t eye_idx = 0, uint32_t camera_offset = 0);

    void process_events();

    void submit_global_command_encoder();

    void set_camera_type(eCameraType camera_type);
    eCameraType get_camera_type();

    void set_custom_pass_user_data(void* user_data);

    void increase_frame_counter() { frame_counter++; }
    uint32_t get_frame_counter() { return frame_counter; }

    void init_lighting_bind_group();
    WGPUBindGroup get_lighting_bind_group() { return lighting_bind_group; }
    WGPUBindGroup get_render_camera_bind_group() { return render_camera_bind_group; }
    WGPUBindGroup get_compute_camera_bind_group() { return compute_camera_bind_group; }

    void init_depth_buffers();
    void init_multisample_textures();
    void init_timestamp_queries();

    void set_frustum_camera_paused(bool value);
    bool get_frustum_camera_paused();

    bool get_use_custom_mirror() { return use_custom_mirror; }

    void request_timestamps() { timestamps_requested = true; }

    WGPUCommandEncoder get_global_command_encoder() { return global_command_encoder; }

    void resolve_query_set(WGPUCommandEncoder encoder, uint8_t first_query);
    std::vector<float>& get_last_frame_timestamps() { return last_frame_timestamps; }

    void set_msaa_count(uint8_t msaa_count, bool is_initial_value = false);
    uint8_t get_msaa_count();

    bool is_inside_frustum(const glm::vec3& minp, const glm::vec3& maxp) const;

    void prepare_cull_instancing(const Camera& camera, std::vector<std::vector<sRenderData>>& render_lists, sInstanceData& instances_data, bool is_shadow_pass = false);
    void render_opaque(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_transparent(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_splats(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride = 0);
    void render_2D(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group);

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
    void render_mirror(WGPUTextureView screen_surface_texture_view, WGPUBindGroup displayed_fbo_bind_group);
    void init_mirror_pipeline();

    void set_custom_mirror_fbo_bind_group(WGPUBindGroup fbo_bind_group) { custom_mirror_fbo_bind_group = fbo_bind_group; }

    Pipeline mirror_pipeline;
    Shader* mirror_shader = nullptr;
    WGPUBindGroup custom_mirror_fbo_bind_group = nullptr;

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

    void set_required_features(std::vector<WGPUFeatureName> new_required_features) { required_features = new_required_features; };
    void set_required_limits(const WGPURequiredLimits& required_limits) { webgpu_context->required_limits = required_limits; }

    void add_renderable(MeshInstance* mesh_instance, const glm::mat4x4& global_matrix);
    void add_splat_scene(GSNode* gs_scene);
    void clear_renderables();

    void update_lights();
    void add_light(Light3D* new_light);

    virtual void resize_window(int width, int height);

    GLFWwindow* get_glfw_window();

    void set_irradiance_texture(Texture* texture);
    Texture* get_irradiance_texture() { return irradiance_texture; }

    Camera* get_camera() { return camera_3d; }

    void store_texture_to_disk(WGPUCommandEncoder cmd_encoder, const WGPUTexture gpu_texture, const WGPUExtent3D in_size, const char* file_dir);
};
