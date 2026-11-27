#include "gaussian_splats_storage.h"

#include "core/managers/engine/engine_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/render/storage/render_storage.h"
#include "core/managers/xr/xr_manager.h"

#include "shaders/gaussian_splatting/gs_basis.wgsl.gen.h"
#include "shaders/gaussian_splatting/gs_covariance.wgsl.gen.h"
#include "shaders/gaussian_splatting/gs_render.wgsl.gen.h"

Error GaussianSplatsStorage::initialize()
{
    covariance_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::gs_covariance::source, shaders::gs_covariance::path, shaders::gs_covariance::libraries);
    covariance_pipeline.create_compute(covariance_shader);

    basis_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::gs_basis::source, shaders::gs_basis::path, shaders::gs_basis::libraries);
    basis_pipeline.create_compute_async(basis_shader);

    bool xr_available = XR_MANAGER->is_xr_available();
    WGPUTextureFormat swapchain_format = xr_available ? XR_MANAGER->get_swapchain_format() : RENDER_MANAGER->get_surface_format();

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = nullptr;
    color_target.writeMask = WGPUColorWriteMask_All;

    RenderPipelineDescription desc = { .topology = WGPUPrimitiveTopology_TriangleStrip };

    WGPUBlendState* blend_state = new WGPUBlendState;
    blend_state->color = {
        .operation = WGPUBlendOperation_Add,
        .srcFactor = WGPUBlendFactor_SrcAlpha,
        .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
    };
    blend_state->alpha = {
        .operation = WGPUBlendOperation_Add,
        .srcFactor = WGPUBlendFactor_Zero,
        .dstFactor = WGPUBlendFactor_One,
    };

    color_target.blend = blend_state;

    desc.depth_write = WGPUOptionalBool_False;
    desc.blending_enabled = true;
    desc.sample_count = EngineManager::get_singleton()->get_configuration().render_config.msaa_count;

    splats_render_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::gs_render::source, shaders::gs_render::path, shaders::gs_render::libraries);
    splats_render_pipeline.create_render_async(splats_render_shader, color_target, desc);

    return Error::OK;
}

void GaussianSplatsStorage::create_buffers(SplatData* splat_data)
{
    splat_data->buffers = new SplatBuffers();
    SplatBuffers* buffers = static_cast<SplatBuffers*>(splat_data->buffers);
    buffers->initialize(splat_data, covariance_shader, basis_shader, splats_render_shader);

    WGPUCommandEncoder command_encoder = RENDER_MANAGER->get_main_command_encoder();

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;

    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    covariance_pipeline.set(compute_pass);

    // Calculate covariance
    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, buffers->covariance_bindgroup, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, buffers->workgroup_size, 1, 1);

    wgpuComputePassEncoderEnd(compute_pass);
    wgpuComputePassEncoderRelease(compute_pass);
}

const Pipeline& GaussianSplatsStorage::get_render_pipeline() const
{
    return splats_render_pipeline;
}

const Pipeline& GaussianSplatsStorage::get_basis_pipeline() const
{
    return basis_pipeline;
}

const Pipeline& GaussianSplatsStorage::get_covariance_pipeline() const
{
    return covariance_pipeline;
}

GaussianSplatsStorage::SplatBuffers::~SplatBuffers()
{
    RENDER_API->buffer_release(render_buffer);
    RENDER_API->buffer_release(ids_buffer);

    RENDER_API->bind_group_release(render_bindgroup);
    RENDER_API->bind_group_release(covariance_bindgroup);
    RENDER_API->bind_group_release(basis_uniform_bindgroup);

    delete radix_sort_kernel;
}

void GaussianSplatsStorage::SplatBuffers::initialize(SplatData* splat_data, Shader* covariance_shader, Shader* basis_shader, Shader* splats_render_shader)
{
    splat_count = static_cast<uint32_t>(splat_data->positions.size());
    padded_splat_count = static_cast<uint32_t>(ceil(static_cast<float>(splat_count) / 512.0f) * 512);
    workgroup_size = static_cast<uint32_t>(ceil(static_cast<float>(padded_splat_count) / 256.0f));

    float splat_positions[8] = {
        1.0f, 1.0f,
        -1.0f, 1.0f,
        1.0f, -1.0f,
        -1.0f, -1.0f
    };

    std::string name = std::string(splat_data->name);
    render_buffer = RenderAPI::get_singleton()->buffer_create(sizeof(float) * 8, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, splat_positions, ("gs_buffer_" + name).c_str());
    ids_buffer = RenderAPI::get_singleton()->buffer_create(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, nullptr, ("ids_buffer_" + name).c_str());

    // Covariance
    {
        rotations_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(glm::quat) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, splat_data->rotations.data(), "rotations_buffer");
        rotations_uniform.binding = 0;
        rotations_uniform.buffer_size = sizeof(glm::quat) * splat_count;

        scales_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, splat_data->scales.data(), "scales_buffer");
        scales_uniform.binding = 1;
        scales_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        covariance_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(glm::vec3) * splat_count * 8, WGPUBufferUsage_Storage, nullptr, "covariance_buffer");
        covariance_uniform.binding = 2;
        covariance_uniform.buffer_size = sizeof(glm::vec3) * splat_count * 8;

        splat_count_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &splat_count, "splat_count_buffer");
        splat_count_uniform.binding = 6;
        splat_count_uniform.buffer_size = sizeof(uint32_t);

        std::vector<Uniform*> uniforms = { &rotations_uniform, &scales_uniform, &covariance_uniform, &splat_count_uniform };
        covariance_bindgroup = RenderAPI::get_singleton()->bind_group_create(uniforms, covariance_shader, 0);
    }

    // Render
    {
        model_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(glm::mat4x4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &splat_data->global_matrix[0], "model_matrix_buffer");
        model_uniform.binding = 0;
        model_uniform.buffer_size = sizeof(glm::mat4x4);

        centroid_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, splat_data->positions.data(), "centroids_buffer");
        centroid_uniform.binding = 1;
        centroid_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        color_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, splat_data->colors.data(), "colors_buffer");
        color_uniform.binding = 2;
        color_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        basis_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_Storage, nullptr, "basis_buffer");
        basis_uniform.binding = 3;
        basis_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        std::vector<Uniform*> uniforms = { &model_uniform, &centroid_uniform, &basis_uniform, &color_uniform };
        render_bindgroup = RenderAPI::get_singleton()->bind_group_create(uniforms, splats_render_shader, 0);
    }

    // Basis
    {
        distances_buffer = RenderAPI::get_singleton()->buffer_create(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_Storage, nullptr, "distances_buffer");

        basis_distances_uniform.data = distances_buffer;
        basis_distances_uniform.binding = 4;
        basis_distances_uniform.buffer_size = sizeof(uint32_t) * padded_splat_count;

        ids_basis_uniform.data = ids_buffer;
        ids_basis_uniform.binding = 5;
        ids_basis_uniform.buffer_size = sizeof(uint32_t) * padded_splat_count;

        std::vector<Uniform*> uniforms = { &model_uniform, &centroid_uniform, &covariance_uniform, &basis_uniform, &basis_distances_uniform, &ids_basis_uniform, &splat_count_uniform };
        basis_uniform_bindgroup = RenderAPI::get_singleton()->bind_group_create(uniforms, basis_shader, 0);
    }

    radix_sort_kernel = new RadixSortKernel(distances_buffer, sizeof(uint32_t) * padded_splat_count, ids_buffer, sizeof(uint32_t) * padded_splat_count, padded_splat_count);
}
