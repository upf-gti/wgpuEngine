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

bool AABB::initialized() const
{
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
}

AABB AABB::rotate(const glm::quat& rotation) const
{
    const glm::quat& inv_rotation = glm::inverse(rotation);

    const glm::vec3 axis[8] = { inv_rotation * glm::vec3(half_size.x,  half_size.y,  half_size.z),
                                inv_rotation * glm::vec3(half_size.x,  half_size.y, -half_size.z),
                                inv_rotation * glm::vec3(half_size.x, -half_size.y,  half_size.z),
                                inv_rotation * glm::vec3(half_size.x, -half_size.y, -half_size.z),
                                inv_rotation * glm::vec3(-half_size.x,  half_size.y,  half_size.z),
                                inv_rotation * glm::vec3(-half_size.x,  half_size.y, -half_size.z),
                                inv_rotation * glm::vec3(-half_size.x, -half_size.y,  half_size.z),
                                inv_rotation * glm::vec3(-half_size.x, -half_size.y, -half_size.z) };

    glm::vec3 new_min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 new_max = glm::vec3(std::numeric_limits<float>::lowest());

    for (const glm::vec3& corner : axis) {
        new_min = glm::min(new_min, corner);
        new_max = glm::max(new_max, corner);
    }

    const glm::vec3 new_half_size = (new_max - new_min) * 0.5f;

    return { center, new_half_size };
}

bool AABB::ray_intersection(glm::dvec3 ray_origin, glm::dvec3 ray_dir, double min_interval, double max_interval) const
{
    glm::dvec3 t_min = (static_cast<glm::dvec3>(center - half_size) - ray_origin) / ray_dir;
    glm::dvec3 t_max = (static_cast<glm::dvec3>(center + half_size) - ray_origin) / ray_dir;
    glm::dvec3 t1 = glm::min(t_min, t_max);
    glm::dvec3 t2 = glm::max(t_min, t_max);
    double t_near = std::max(std::max(t1.x, t1.y), t1.z);
    double t_far = std::min(std::min(t2.x, t2.y), t2.z);

    return t_near <= t_far && t_near > min_interval && t_far < max_interval;
}
