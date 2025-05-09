#pragma once

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/quaternion.hpp"

#include "framework/math/math_utils.h"

#include <algorithm>

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

    AABB rotate(const glm::quat& rotation) const;

    // Returns the index of the longest axis of the bounding box.
    int longest_axis() const {
        float x_size = abs(half_size.x);
        float y_size = abs(half_size.y);
        float z_size = abs(half_size.z);

        if (x_size > y_size)
            return x_size > z_size ? 0 : 2;
        else
            return y_size > z_size ? 1 : 2;
    }
};

AABB merge_aabbs(const AABB& AABB_0, const AABB& AABB_1);
