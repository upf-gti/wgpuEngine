#pragma once

#include "core/error/error.h"

#include <webgpu/webgpu.h>

#include <functional>
#include <string>
#include <vector>

struct Uniform;
class Texture;
class Shader;
class Pipeline;

struct RenderPipelineDescription {
    std::string vs_entry_point = "vs_main";
    std::string fs_entry_point = "fs_main";

    WGPUCullMode cull_mode = WGPUCullMode_None;
    WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList;

    bool use_depth = true;
    bool depth_read = true;
    bool has_fragment_state = true;
    WGPUOptionalBool depth_write = WGPUOptionalBool_True;

    WGPUCompareFunction depth_compare = WGPUCompareFunction_Greater;

    bool blending_enabled = false;
    uint8_t sample_count = 1;
    bool allow_msaa = true;
};

WGPUStringView get_string_view(const char* str);

class RenderAPI {
    friend class RenderManager;

public:
    inline static RenderAPI* get_singleton()
    {
        return singleton_instance;
    }

    inline WGPUDevice get_device() const { return device; }
    const WGPULimits& get_supported_limits() const { return supported_limits; }

    void process_events();

    // Shaders
    WGPUShaderModule create_shader_module(char const* code);

    // Buffers
    WGPUVertexBufferLayout create_vertex_buffer_layout(const std::vector<WGPUVertexAttribute>& vertex_attributes, uint64_t stride, WGPUVertexStepMode step_mode);
    WGPUBuffer buffer_create(size_t size, int usage, const void* data, const char* label = nullptr);
    void buffer_update(WGPUBuffer buffer, uint64_t buffer_offset, void const* data, size_t size);
    void buffer_read(WGPUBuffer buffer, size_t size, void* output_data);
    void buffer_read_async(WGPUBuffer buffer, size_t size, const std::function<void(const void* output_buffer, void* userdata)>& read_callback, void* read_userdata);

    // Textures
    WGPUTexture texture_create(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, uint8_t sample_count, const char* label = "");
    WGPUTextureView texture_view_create(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect = WGPUTextureAspect_All, uint32_t base_mip_level = 0, uint32_t mip_level_count = 1, uint32_t base_array_layer = 0, uint32_t array_layer_count = 1, const char* label = "") const;
    WGPUSampler sampler_create(WGPUAddressMode wrap_u = WGPUAddressMode_ClampToEdge, WGPUAddressMode wrap_v = WGPUAddressMode_ClampToEdge, WGPUAddressMode wrap_w = WGPUAddressMode_ClampToEdge,
            WGPUFilterMode mag_filter = WGPUFilterMode_Linear, WGPUFilterMode min_filter = WGPUFilterMode_Linear, WGPUMipmapFilterMode mipmap_filter = WGPUMipmapFilterMode_Linear,
            float lod_max_clamp = 1.0f, uint16_t max_anisotropy = 1u, WGPUCompareFunction compare_function = WGPUCompareFunction_Undefined);
    void texture_mipmaps_create(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, WGPUTextureViewDimension view_dimension = WGPUTextureViewDimension_2D, WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm, WGPUOrigin3D origin = { 0, 0, 0 }, WGPUCommandEncoder custom_command_encoder = nullptr);
    void cubemap_mipmaps_create(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, WGPUTextureViewDimension view_dimension = WGPUTextureViewDimension_2D, WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm, WGPUOrigin3D origin = { 0, 0, 0 }, WGPUCommandEncoder custom_command_encoder = nullptr);
    void texture_upload(WGPUTexture texture, WGPUTextureDimension dimension, WGPUExtent3D texture_size, uint32_t mip_level, WGPUTextureFormat format, const void* data, WGPUOrigin3D origin = { 0, 0, 0 });
    void texture_copy(WGPUTexture texture_src, WGPUTexture texture_dst, uint32_t src_mipmap_level, uint32_t dst_mipmap_level, const WGPUExtent3D& copy_size, const WGPUOrigin3D& src_origin = { 0, 0, 0 }, const WGPUOrigin3D& dst_origin = { 0, 0, 0 }, WGPUCommandEncoder custom_command_encoder = nullptr);

    // Bind groups
    WGPUBindGroupLayout create_bind_group_layout(const std::vector<WGPUBindGroupLayoutEntry>& entries, char const* label = nullptr);
    WGPUBindGroup bind_group_create(const std::vector<Uniform*>& uniforms, WGPUBindGroupLayout bind_group_layout, char const* label = nullptr);
    WGPUBindGroup bind_group_create(const std::vector<Uniform*>& uniforms, const Shader* shader, uint16_t bind_group, char const* label = nullptr) const;

    // Pipelines
    WGPUPipelineLayout create_pipeline_layout(const std::vector<WGPUBindGroupLayout>& bind_group_layouts, const std::string& label = "");
    WGPURenderPipeline render_pipeline_create(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes,
            WGPUColorTargetState color_target, const RenderPipelineDescription& description, std::vector<WGPUConstantEntry> constants = {});
    void render_pipeline_create_async(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes,
            WGPUColorTargetState color_target, WGPUCreateRenderPipelineAsyncCallbackInfo callback_info, const RenderPipelineDescription& description,
            std::vector<WGPUConstantEntry> constants = {});
    WGPUComputePipeline compute_pipeline_create(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout, const char* entry_point = "compute", std::vector<WGPUConstantEntry> constants = {});
    void compute_pipeline_create_async(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout, WGPUCreateComputePipelineAsyncCallbackInfo callback_info,
            const char* entry_point = "compute", std::vector<WGPUConstantEntry> constants = {});

    WGPUQuerySet create_query_set(uint8_t maximum_query_sets, WGPUStringView label);

    void push_debug_group(WGPURenderPassEncoder render_pass, WGPUStringView label);
    void push_debug_group(WGPUComputePassEncoder compute_pass, WGPUStringView label);

    void pop_debug_group(WGPURenderPassEncoder render_pass);
    void pop_debug_group(WGPUComputePassEncoder compute_pass);

private:
    static inline RenderAPI* singleton_instance = nullptr;

    RenderAPI() = default;
    ~RenderAPI() = default;

    Error initialize();
    Error finalize();

    WGPUFuture request_adapter();
    WGPUFuture request_device(const std::vector<WGPUFeatureName>& required_features, const WGPULimits& required_limits);

    void api_instance_create();
    void surface_configure_swapchain(int width, int height);

    void print_device_info();

    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;

    WGPUSurface surface = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue device_queue = nullptr;

    WGPUTextureFormat surface_format = WGPUTextureFormat_Undefined;

    WGPULimits supported_limits;

    struct sMipmapPipeline {
        Pipeline* mipmap_pipeline;
        Shader* mipmap_shader;
    };

    sMipmapPipeline cubemap_mipmap_pipeline;
    std::unordered_map<WGPUTextureFormat, sMipmapPipeline> mipmap_pipelines;

    sMipmapPipeline get_mipmap_pipeline(WGPUTextureFormat texture_format);
};
