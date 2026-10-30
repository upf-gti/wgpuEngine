#include "graphics_utils.h"

#include "core/managers/render/render_api.h"

glm::uvec2 find_optimal_dispatch_size(uint32_t workgroup_count)
{
    glm::uvec2 dispatch_size = { workgroup_count, 1 };

    if (workgroup_count > RenderAPI::get_singleton()->get_supported_limits().maxComputeWorkgroupsPerDimension) {
        uint32_t x = static_cast<uint32_t>(floor(sqrt(workgroup_count)));
        uint32_t y = static_cast<uint32_t>(ceil(workgroup_count / x));

        dispatch_size.x = x;
        dispatch_size.y = y;
    }

    return dispatch_size;
}
