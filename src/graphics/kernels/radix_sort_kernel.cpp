#include "radix_sort_kernel.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"
#include "graphics/graphics_utils.h"

#include "shaders/kernels/radix_sort.wgsl.gen.h"
#include "shaders/kernels/radix_sort_reorder.wgsl.gen.h"

RadixSortKernel::RadixSortKernel(WGPUBuffer keys_buffer, uint32_t keys_byte_size, WGPUBuffer values_buffer, uint32_t values_byte_size, uint32_t count, uint32_t bit_count, const glm::uvec2& workgroup_size) :
    keys_buffer(keys_buffer), keys_byte_size(keys_byte_size),
    values_buffer(values_buffer), values_byte_size(values_byte_size),
    count(count), bit_count(bit_count), workgroup_size(workgroup_size)
{
    threads_per_workgroup = workgroup_size.x * workgroup_size.y;
    workgroup_count = ceil(count / static_cast<float>(threads_per_workgroup));
    prefix_block_workgroup_count = 4 * workgroup_count;

    block_sum_shader = RendererStorage::get_shader_from_source(shaders::radix_sort::source, shaders::radix_sort::path, shaders::radix_sort::libraries);
    reorder_shader = RendererStorage::get_shader_from_source(shaders::radix_sort_reorder::source, shaders::radix_sort_reorder::path, shaders::radix_sort_reorder::libraries);

    create_pipelines();
}

void RadixSortKernel::create_pipelines()
{
    create_prefix_sum_kernel();

    calculate_dispatch_sizes();

    create_buffers();

    for (uint8_t bit = 0; bit < bit_count; bit += 2) {
        // Swap buffers every pass
        bool even = (bit % 4 == 0);
        WGPUBuffer in_keys = even ? keys_buffer : tmp_keys_buffer;
        WGPUBuffer in_values = even ? values_buffer : tmp_values_buffer;
        WGPUBuffer out_keys = even ? tmp_keys_buffer : keys_buffer;
        WGPUBuffer out_values = even ? tmp_values_buffer : values_buffer;

        // Compute local prefix sums and block sums
        PipelineData* block_sum_pipeline = create_block_sum_pipeline(in_keys, bit);

        // Reorder keys and values
        PipelineData* reorder_pipeline = create_reorder_pipeline(in_keys, in_values, out_keys, out_values, bit);

        pipelines.push_back(block_sum_pipeline); 
        pipelines.push_back(reorder_pipeline);
    }
}

void RadixSortKernel::calculate_dispatch_sizes()
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();
    dispatch_size = find_optimal_dispatch_size(webgpu_context, workgroup_count);
}

void RadixSortKernel::create_prefix_sum_kernel()
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    prefix_block_sum_buffer = webgpu_context->create_buffer(prefix_block_workgroup_count * 4,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, nullptr,
        "radix_sort_prefix_block_sum");

    prefix_sum_kernel = new PrefixSumKernel(prefix_block_sum_buffer, prefix_block_workgroup_count * 4, prefix_block_workgroup_count, workgroup_size);
}

RadixSortKernel::PipelineData* RadixSortKernel::create_block_sum_pipeline(WGPUBuffer in_keys, uint8_t bit)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    PipelineData* pipeline_data = new PipelineData();

    pipeline_data->in_keys_uniform.data = in_keys;
    pipeline_data->in_keys_uniform.binding = 0;
    pipeline_data->in_keys_uniform.buffer_size = keys_byte_size;

    pipeline_data->local_prefix_uniform.data = local_prefix_sum_buffer;
    pipeline_data->local_prefix_uniform.binding = 1;
    pipeline_data->local_prefix_uniform.buffer_size = count * 4;

    pipeline_data->prefix_block_uniform.data = prefix_block_sum_buffer;
    pipeline_data->prefix_block_uniform.binding = 2;
    pipeline_data->prefix_block_uniform.buffer_size = prefix_block_workgroup_count * 4;

    std::vector<Uniform*> uniforms = { &pipeline_data->in_keys_uniform, &pipeline_data->local_prefix_uniform, &pipeline_data->prefix_block_uniform };

    pipeline_data->bind_group = webgpu_context->create_bind_group(uniforms, block_sum_shader, 0, "radix_sort_block_sum_bind_group");

    std::vector<WGPUConstantEntry> constants = {
        { nullptr, get_string_view("WORKGROUP_SIZE_X"), static_cast<double>(workgroup_size.x) },
        { nullptr, get_string_view("WORKGROUP_SIZE_Y"), static_cast<double>(workgroup_size.y) },
        { nullptr, get_string_view("WORKGROUP_COUNT"),static_cast<double>(workgroup_count) },
        { nullptr, get_string_view("THREADS_PER_WORKGROUP"), static_cast<double>(threads_per_workgroup) },
        { nullptr, get_string_view("ELEMENT_COUNT"), static_cast<double>(count) },
        { nullptr, get_string_view("CURRENT_BIT"), static_cast<double>(bit) },
    };

    pipeline_data->pipeline.create_compute_async(block_sum_shader, "radix_sort", constants);

    return pipeline_data;
}

RadixSortKernel::PipelineData* RadixSortKernel::create_reorder_pipeline(WGPUBuffer in_keys, WGPUBuffer in_values, WGPUBuffer out_keys, WGPUBuffer out_values, uint8_t bit)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    PipelineData* pipeline_data = new PipelineData();

    pipeline_data->in_keys_uniform.data = in_keys;
    pipeline_data->in_keys_uniform.binding = 0;
    pipeline_data->in_keys_uniform.buffer_size = keys_byte_size;

    pipeline_data->out_keys_uniform.data = out_keys;
    pipeline_data->out_keys_uniform.binding = 1;
    pipeline_data->out_keys_uniform.buffer_size = keys_byte_size;

    pipeline_data->local_prefix_uniform.data = local_prefix_sum_buffer;
    pipeline_data->local_prefix_uniform.binding = 2;
    pipeline_data->local_prefix_uniform.buffer_size = count * 4;

    pipeline_data->prefix_block_uniform.data = prefix_block_sum_buffer;
    pipeline_data->prefix_block_uniform.binding = 3;
    pipeline_data->prefix_block_uniform.buffer_size = prefix_block_workgroup_count * 4;

    pipeline_data->in_values_uniform.data = in_values;
    pipeline_data->in_values_uniform.binding = 4;
    pipeline_data->in_values_uniform.buffer_size = values_byte_size;

    pipeline_data->out_values_uniform.data = out_values;
    pipeline_data->out_values_uniform.binding = 5;
    pipeline_data->out_values_uniform.buffer_size = values_byte_size;

    std::vector<Uniform*> uniforms = {
        &pipeline_data->in_keys_uniform, &pipeline_data->out_keys_uniform,
        &pipeline_data->local_prefix_uniform, &pipeline_data->prefix_block_uniform,
        &pipeline_data->in_values_uniform, &pipeline_data->out_values_uniform
    };

    pipeline_data->bind_group = webgpu_context->create_bind_group(uniforms, reorder_shader, 0, "radix_sort_reorder_bind_group");

    std::vector<WGPUConstantEntry> constants = {
        { nullptr, get_string_view("WORKGROUP_SIZE_X"), static_cast<double>(workgroup_size.x) },
        { nullptr, get_string_view("WORKGROUP_SIZE_Y"), static_cast<double>(workgroup_size.y) },
        { nullptr, get_string_view("WORKGROUP_COUNT"), static_cast<double>(workgroup_count) },
        { nullptr, get_string_view("THREADS_PER_WORKGROUP"), static_cast<double>(threads_per_workgroup) },
        { nullptr, get_string_view("ELEMENT_COUNT"), static_cast<double>(count) },
        { nullptr, get_string_view("CURRENT_BIT"), static_cast<double>(bit) },
    };

    pipeline_data->pipeline.create_compute_async(reorder_shader, "radix_sort_reorder", constants);

    return pipeline_data;
}

void RadixSortKernel::create_buffers()
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    tmp_keys_buffer = webgpu_context->create_buffer(keys_byte_size,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, nullptr,
        "radix_sort_tmp_keys");

    tmp_values_buffer = webgpu_context->create_buffer(values_byte_size,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, nullptr,
        "radix_sort_tmp_values");

    local_prefix_sum_buffer = webgpu_context->create_buffer(count * sizeof(uint32_t),
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, nullptr,
        "radix_sort_local_prefix_sum");
}

RadixSortKernel::~RadixSortKernel()
{
    for (PipelineData* pipeline_data : pipelines) {
        pipeline_data->in_keys_uniform.destroy();
        pipeline_data->in_values_uniform.destroy();
        pipeline_data->local_prefix_uniform.destroy();
        pipeline_data->out_keys_uniform.destroy();
        pipeline_data->out_values_uniform.destroy();
        pipeline_data->prefix_block_uniform.destroy();

        wgpuBindGroupRelease(pipeline_data->bind_group);

        delete pipeline_data;
    }

    if (prefix_sum_kernel) {
        delete prefix_sum_kernel;
    }
}

void RadixSortKernel::dispatch(WGPUComputePassEncoder compute_pass)
{
    for (uint8_t i = 0; i < bit_count; i += 2) {
        PipelineData* block_sum_pipeline = pipelines[i];
        PipelineData* reorder_pipeline = pipelines[i + 1];

        if (!block_sum_pipeline->pipeline.set(compute_pass)) {
            return;
        }

        wgpuComputePassEncoderSetBindGroup(compute_pass, 0, block_sum_pipeline->bind_group, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(compute_pass, dispatch_size.x, dispatch_size.y, 1);

        prefix_sum_kernel->dispatch(compute_pass);

        if (!reorder_pipeline->pipeline.set(compute_pass)) {
            return;
        }

        wgpuComputePassEncoderSetBindGroup(compute_pass, 0, reorder_pipeline->bind_group, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(compute_pass, dispatch_size.x, dispatch_size.y, 1);
    }
}
