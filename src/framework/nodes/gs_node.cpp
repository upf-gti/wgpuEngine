#include "gs_node.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "framework/math/math_utils.h"

#include "shaders/gaussian_splatting/gs_render.wgsl.gen.h"
#include "shaders/gaussian_splatting/gs_covariance.wgsl.gen.h"
#include "shaders/gaussian_splatting/gs_basis.wgsl.gen.h"

GSNode::GSNode()
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    covariance_shader = RendererStorage::get_shader_from_source(shaders::gs_covariance::source, shaders::gs_covariance::path);
    covariance_pipeline.create_compute_async(covariance_shader);

    basis_shader = RendererStorage::get_shader_from_source(shaders::gs_basis::source, shaders::gs_basis::path);
    basis_pipeline.create_compute_async(basis_shader);
}

GSNode::~GSNode()
{
    if (radix_sort_kernel) {
        delete radix_sort_kernel;
    }
}

void GSNode::initialize(uint32_t splat_count)
{
    this->splat_count = splat_count;
    this->padded_splat_count = ceil(static_cast<float>(splat_count) / 512.0f) * 512;
    this->workgroup_size = ceil(static_cast<float>(padded_splat_count) / 256.0f);

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    float splat_positions[8] = {
         1.0f,  1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
        -1.0f, -1.0f
    };

    render_buffer = webgpu_context->create_buffer(sizeof(float) * 8, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, splat_positions, ("gs_buffer_" + name).c_str());
    ids_buffer = webgpu_context->create_buffer(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage, nullptr, ("ids_buffer_" + name).c_str());

    // Covariance
    {
        rotations_uniform.data = webgpu_context->create_buffer(sizeof(glm::quat) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "rotations_buffer");
        rotations_uniform.binding = 0;
        rotations_uniform.buffer_size = sizeof(glm::quat) * splat_count;

        scales_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "scales_buffer");
        scales_uniform.binding = 1;
        scales_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        covariance_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec3) * splat_count * 8, WGPUBufferUsage_Storage, nullptr, "covariance_buffer");
        covariance_uniform.binding = 2;
        covariance_uniform.buffer_size = sizeof(glm::vec3) * splat_count * 8;

        splat_count_uniform.data = webgpu_context->create_buffer(sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &splat_count, "splat_count_buffer");
        splat_count_uniform.binding = 6;
        splat_count_uniform.buffer_size = sizeof(uint32_t);

        std::vector<Uniform*> uniforms = { &rotations_uniform, &scales_uniform, &covariance_uniform, &splat_count_uniform };
        covariance_bindgroup = webgpu_context->create_bind_group(uniforms, covariance_shader, 0);
    }

    // Render
    {
        model_uniform.data = webgpu_context->create_buffer(sizeof(glm::mat4x4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &get_global_model()[0], "model_matrix_buffer");
        model_uniform.binding = 0;
        model_uniform.buffer_size = sizeof(glm::mat4x4);

        centroid_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "centroids_buffer");
        centroid_uniform.binding = 1;
        centroid_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        color_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "colors_buffer");
        color_uniform.binding = 2;
        color_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        basis_uniform.data = webgpu_context->create_buffer(sizeof(glm::vec4) * splat_count, WGPUBufferUsage_Storage, nullptr, "basis_buffer");
        basis_uniform.binding = 3;
        basis_uniform.buffer_size = sizeof(glm::vec4) * splat_count;

        std::vector<Uniform*> uniforms = { &model_uniform, &centroid_uniform, &basis_uniform, &color_uniform };
        render_bindgroup = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader_from_source(shaders::gs_render::source, shaders::gs_render::path), 0);
    }

    // Basis
    {
        distances_buffer = webgpu_context->create_buffer(sizeof(uint32_t) * padded_splat_count, WGPUBufferUsage_Storage, nullptr, "distances_buffer");

        basis_distances_uniform.data = distances_buffer;
        basis_distances_uniform.binding = 4;
        basis_distances_uniform.buffer_size = sizeof(uint32_t) * padded_splat_count;

        ids_basis_uniform.data = ids_buffer;
        ids_basis_uniform.binding = 5;
        ids_basis_uniform.buffer_size = sizeof(uint32_t) * padded_splat_count;

        std::vector<Uniform*> uniforms = { &model_uniform, &centroid_uniform, &covariance_uniform, &basis_uniform, &basis_distances_uniform, &ids_basis_uniform, &splat_count_uniform };
        basis_uniform_bindgroup = webgpu_context->create_bind_group(uniforms, basis_shader, 0);
    }

    radix_sort_kernel = new RadixSortKernel(distances_buffer, sizeof(uint32_t) * padded_splat_count, ids_buffer, sizeof(uint32_t) * padded_splat_count, padded_splat_count);
}

void GSNode::render()
{
    Renderer::instance->add_splat_scene(this);
}

void GSNode::update(float delta_time)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();
    webgpu_context->update_buffer(std::get<WGPUBuffer>(model_uniform.data), 0, &get_global_model()[0], sizeof(glm::mat4x4));

    WGPUCommandEncoder command_encoder = Renderer::instance->get_global_command_encoder();

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;

    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    if (!covariance_calculated) {
        calculate_covariance(compute_pass);
    }

    calculate_basis(compute_pass);
    sort(compute_pass);

    wgpuComputePassEncoderEnd(compute_pass);
    wgpuComputePassEncoderRelease(compute_pass);
}

void GSNode::sort(WGPUComputePassEncoder compute_pass)
{
    radix_sort_kernel->dispatch(compute_pass);
}

void GSNode::set_covariance_buffers(const std::vector<glm::quat>& rotations, const std::vector<glm::vec4>& scales)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    webgpu_context->update_buffer(std::get<WGPUBuffer>(rotations_uniform.data), 0, rotations.data(), sizeof(glm::quat) * splat_count);
    webgpu_context->update_buffer(std::get<WGPUBuffer>(scales_uniform.data), 0, scales.data(), sizeof(glm::vec4) * splat_count);
}

void GSNode::calculate_covariance(WGPUComputePassEncoder compute_pass)
{
    if (!covariance_pipeline.set(compute_pass)) {
        return;
    }

    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, covariance_bindgroup, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroup_size, 1, 1);

    covariance_calculated = true;
}

void GSNode::calculate_basis(WGPUComputePassEncoder compute_pass)
{
    uint32_t camera_stride_offset = 0;

    if (!basis_pipeline.set(compute_pass)) {
        return;
    }

    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, basis_uniform_bindgroup, 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(compute_pass, 1, Renderer::instance->get_compute_camera_bind_group(), 1, &camera_stride_offset);
    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroup_size, 1, 1);
}

void GSNode::set_render_buffers(const std::vector<glm::vec4>& positions, const std::vector<glm::vec4>& colors)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    webgpu_context->update_buffer(std::get<WGPUBuffer>(centroid_uniform.data), 0, positions.data(), sizeof(glm::vec4) * splat_count);
    webgpu_context->update_buffer(std::get<WGPUBuffer>(color_uniform.data), 0, colors.data(), sizeof(glm::vec4) * splat_count);
}

uint32_t GSNode::get_splat_count()
{
    return splat_count;
}

uint32_t GSNode::get_padded_splat_count()
{
    return padded_splat_count;
}

uint32_t GSNode::get_splats_render_bytes_size()
{
    return sizeof(float) * 8;
}

uint32_t GSNode::get_ids_render_bytes_size()
{
    return sizeof(uint32_t) * padded_splat_count;
}
