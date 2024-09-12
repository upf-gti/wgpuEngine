#pragma once

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

struct AABB {
    glm::vec3 center = {};
    glm::vec3 half_size = {};

    bool initialized() const;

    /**
    * apply a matrix to a box * note that the resulting box will be axis aligned as well
    * therefore the resulting box may be larger than the previous
    *
    * @param box the box to transform
    * @param mat the trnsformation matrix to apply
    **/

    AABB transform(const glm::mat4& mat) const;
};

AABB merge_aabbs(const AABB& AABB_0, const AABB& AABB_1);
