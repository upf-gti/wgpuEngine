#pragma once

#include "glm/vec3.hpp"

struct AABB {
    glm::vec3 center = {};
    glm::vec3 half_size = {};

    bool initialized() const;
};

AABB merge_aabbs(const AABB& AABB_0, const AABB& AABB_1);
