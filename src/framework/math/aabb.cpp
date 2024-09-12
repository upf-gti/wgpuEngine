#include "aabb.h"

#include "glm/gtx/scalar_relational.hpp"

AABB merge_aabbs(const AABB& AABB_0, const AABB& AABB_1)
{
    if (!AABB_0.initialized()) {
        return AABB_1;
    } else
    if (!AABB_1.initialized()) {
        return AABB_0;
    }

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

inline bool AABB::initialized() const {
    return glm::any(glm::notEqual(center, glm::vec3(0.0f))) || glm::any(glm::notEqual(half_size, glm::vec3(0.0f)));
}

AABB AABB::transform(const glm::mat4& mat) const
{
    float av, bv;
    int   i, j;

    glm::vec3 translation = mat[3];

    glm::vec3 out_aabb_min = { translation.x, translation.y, translation.z };
    glm::vec3 out_aabb_max = { translation.x, translation.y, translation.z };

    glm::vec3 in_aabb_min = center - half_size;
    glm::vec3 in_aabb_max = center + half_size;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            av = mat[j][i] * in_aabb_min[j];
            bv = mat[j][i] * in_aabb_max[j];

            if (av < bv) {
                out_aabb_min[i] += av;
                out_aabb_max[i] += bv;
            }
            else
            {
                out_aabb_min[i] += bv;
                out_aabb_max[i] += av;
            }
        }
    }

    glm::vec3 aabb_half_size = (out_aabb_max - out_aabb_min) * 0.5f;
    glm::vec3 aabb_center = out_aabb_min + aabb_half_size;

    return { aabb_center, aabb_half_size };
};
