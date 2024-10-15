#pragma once

#include "includes.h"
#include "glm/glm.hpp"

#include "graphics/pipeline.h"

#include <vector>

class PrefixSumKernel {

    struct PipelineData {
        Pipeline pipeline;
        Uniform  data_uniform;
        Uniform  block_sum_uniform;
        WGPUBindGroup bind_group;
        glm::uvec2 dispatch_size;
    };

    glm::uvec2 workgroup_size;
    uint32_t threads_per_workgroup;
    uint32_t items_per_workgroup;

    std::vector<PipelineData*> pipelines;
    Shader* shader;

    void create_pass_recursive(WGPUBuffer data, uint32_t data_byte_size, uint32_t count);

public:

    PrefixSumKernel(WGPUBuffer data, uint32_t data_byte_size, uint32_t count, glm::uvec2 workgroup_size = glm::uvec2(16, 16));

};
