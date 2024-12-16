#include "prefix_sum_kernel.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"
#include "graphics/graphics_utils.h"

#include "framework/math/math_utils.h"

#include "shaders/kernels/prefix_sum.wgsl.gen.h"

#include "spdlog/spdlog.h"

PrefixSumKernel::PrefixSumKernel(WGPUBuffer data, uint32_t data_byte_size, uint32_t count, glm::uvec2 workgroup_size) :
    workgroup_size(workgroup_size)
{
    threads_per_workgroup = workgroup_size.x * workgroup_size.y;
    items_per_workgroup = 2 * threads_per_workgroup;

    if (log2(threads_per_workgroup) % 1 != 0) {
        spdlog::error("PrefixSumKernel: workgroup_size.x * workgroup_size.y must be a power of two");
    }

    shader = RendererStorage::get_shader_from_source(shaders::prefix_sum::source, shaders::prefix_sum::path, shaders::prefix_sum::libraries);

    create_pass_recursive(data, data_byte_size, count);
}

PrefixSumKernel::~PrefixSumKernel()
{
    for (PipelineData* pipeline_data : pipelines) {
        delete pipeline_data;
    }
}

std::vector<glm::uvec2> PrefixSumKernel::get_dispatch_chain()
{
    std::vector<glm::uvec2> dispatch_chain;

    for (PipelineData* pipeline_data : pipelines) {
        dispatch_chain.push_back(pipeline_data->dispatch_size);
    }

    return dispatch_chain;
}

void PrefixSumKernel::create_pass_recursive(WGPUBuffer data, uint32_t data_byte_size, uint32_t count)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    const uint32_t workgroup_count = ceil(count / static_cast<float>(items_per_workgroup));

    PipelineData* pipeline_data = new PipelineData();

    pipeline_data->dispatch_size = find_optimal_dispatch_size(webgpu_context, workgroup_count);

    pipeline_data->data_uniform.data = data;
    pipeline_data->data_uniform.binding = 0;
    pipeline_data->data_uniform.buffer_size = data_byte_size;

    pipeline_data->block_sum_uniform.data = webgpu_context->create_buffer(sizeof(uint32_t) * workgroup_count, WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, nullptr, "prefix_sum_block_sum");
    pipeline_data->block_sum_uniform.binding = 1;
    pipeline_data->block_sum_uniform.buffer_size = sizeof(uint32_t) * workgroup_count;

    std::vector<Uniform*> uniforms = { &pipeline_data->data_uniform, &pipeline_data->block_sum_uniform };
    pipeline_data->bind_group = webgpu_context->create_bind_group(uniforms, shader, 0);

    std::vector<WGPUConstantEntry> constants = {
        { nullptr, get_string_view("WORKGROUP_SIZE_X"), static_cast<double>(workgroup_size.x) },
        { nullptr, get_string_view("WORKGROUP_SIZE_Y"), static_cast<double>(workgroup_size.y) },
        { nullptr, get_string_view("THREADS_PER_WORKGROUP"), static_cast<double>(threads_per_workgroup) },
        { nullptr, get_string_view("ITEMS_PER_WORKGROUP"), static_cast<double>(items_per_workgroup) },
        { nullptr, get_string_view("ELEMENT_COUNT"), static_cast<double>(count) },
    };

    pipeline_data->pipeline.create_compute_async(shader, "reduce_downsweep", constants);

    pipelines.push_back(pipeline_data);

    if (workgroup_count > 1) {

        create_pass_recursive(std::get<WGPUBuffer>(pipeline_data->block_sum_uniform.data), sizeof(uint32_t) * workgroup_count, workgroup_count);

        PipelineData* block_sum_pipeline_data = new PipelineData();

        std::vector<WGPUConstantEntry> constants = {
            { nullptr, get_string_view("WORKGROUP_SIZE_X"), static_cast<double>(workgroup_size.x) },
            { nullptr, get_string_view("WORKGROUP_SIZE_Y"), static_cast<double>(workgroup_size.y) },
            { nullptr, get_string_view("THREADS_PER_WORKGROUP"), static_cast<double>(threads_per_workgroup) },
            { nullptr, get_string_view("ELEMENT_COUNT"), static_cast<double>(count) },
        };

        block_sum_pipeline_data->pipeline.create_compute_async(shader, "add_block_sums", constants);
        block_sum_pipeline_data->bind_group = pipeline_data->bind_group;
        block_sum_pipeline_data->dispatch_size = pipeline_data->dispatch_size;

        pipelines.push_back(block_sum_pipeline_data);
    }
}

void PrefixSumKernel::dispatch(WGPUComputePassEncoder compute_pass)
{
    for (PipelineData* pipeline_data : pipelines) {

        pipeline_data->pipeline.set(compute_pass);

        wgpuComputePassEncoderSetBindGroup(compute_pass, 0, pipeline_data->bind_group, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(compute_pass, pipeline_data->dispatch_size.x, pipeline_data->dispatch_size.y, 1);
    }
}
