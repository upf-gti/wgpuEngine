#pragma once

#include "includes.h"
#include "glm/glm.hpp"

#include "graphics/pipeline.h"

#include "prefix_sum_kernel.h"

#include <vector>

class RadixSortKernel {

    struct PipelineData {
        Pipeline pipeline;
        Uniform  in_keys_uniform;
        Uniform  out_keys_uniform;
        Uniform  in_values_uniform;
        Uniform  out_values_uniform;
        Uniform  local_prefix_uniform;
        Uniform  prefix_block_uniform;
        WGPUBindGroup bind_group;
    };

    uint32_t count;
    uint32_t bit_count;
    glm::uvec2 workgroup_size;

    uint32_t keys_byte_size;
    uint32_t values_byte_size;

    uint32_t threads_per_workgroup;
    uint32_t workgroup_count;
    uint32_t prefix_block_workgroup_count;

    glm::uvec2 dispatch_size;

    Shader* block_sum_shader;
    Shader* reorder_shader;

    PrefixSumKernel* prefix_sum_kernel = nullptr;

    WGPUBuffer keys_buffer;
    WGPUBuffer values_buffer;
    WGPUBuffer prefix_block_sum_buffer;

    WGPUBuffer tmp_keys_buffer;
    WGPUBuffer tmp_values_buffer;
    WGPUBuffer local_prefix_sum_buffer;

    std::vector<PipelineData*> pipelines;

    void create_pipelines();
    void create_prefix_sum_kernel();

    void calculate_dispatch_sizes();

    void create_buffers();

    PipelineData* create_block_sum_pipeline(WGPUBuffer in_keys, uint8_t bit);
    PipelineData* create_reorder_pipeline(WGPUBuffer in_keys, WGPUBuffer in_values, WGPUBuffer out_keys, WGPUBuffer out_values, uint8_t bit);

public:

    RadixSortKernel(WGPUBuffer keys_buffer, uint32_t keys_byte_size, WGPUBuffer values_buffer, uint32_t values_byte_size, uint32_t count, uint32_t bit_count = 32, const glm::uvec2& workgroup_size = { 16, 16 });
    ~RadixSortKernel();

    void dispatch(WGPUComputePassEncoder compute_pass);
};
