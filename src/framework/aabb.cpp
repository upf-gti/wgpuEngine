#include "aabb.h"

AABB merge_aabbs(const AABB& AABB_0, const AABB& AABB_1)
{
    glm::vec3 min_0 = AABB_0.center - AABB_0.half_size;
    glm::vec3 max_0 = AABB_0.center + AABB_0.half_size;

    glm::vec3 min_1 = AABB_1.center - AABB_1.half_size;
    glm::vec3 max_1 = AABB_1.center + AABB_1.half_size;

    glm::vec3 new_min = glm::min(min_0, min_1);
    glm::vec3 new_max = glm::max(max_0, max_1);

    glm::vec3 new_half_size = (new_max - new_min) * 0.5f;
    glm::vec3 new_center = new_min + new_half_size;

    return { new_center, new_half_size };
}
