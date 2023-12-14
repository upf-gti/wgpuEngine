#pragma once

#include "glm/glm.hpp"

struct AABB {
    glm::vec3 center = {};
    glm::vec3 half_size = {};

    bool initialized() {
        return glm::any(glm::notEqual(center, glm::vec3(0.0f))) || glm::any(glm::notEqual(half_size, glm::vec3(0.0f)));
    }
};

AABB merge_aabbs(const AABB& AABB_0, const AABB& AABB_1);
