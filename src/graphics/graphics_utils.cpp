#include "graphics_utils.h"

#include "webgpu_context.h"

glm::uvec2 find_optimal_dispatch_size(WebGPUContext* webgpu_context, uint32_t workgroup_count)
{
    glm::uvec2 dispatch_size = { workgroup_count, 1 };

    if (workgroup_count > webgpu_context->supported_limits.maxComputeWorkgroupsPerDimension) {
        uint32_t x = static_cast<uint32_t>(floor(sqrt(workgroup_count)));
        uint32_t y = static_cast<uint32_t>(ceil(workgroup_count / x));

        dispatch_size.x = x;
        dispatch_size.y = y;
    }

    return dispatch_size;
}
