#pragma once

#include "glm/glm.hpp"

struct WebGPUContext;

glm::uvec2 find_optimal_dispatch_size(WebGPUContext* webgpu_context, uint32_t workgroup_count);
