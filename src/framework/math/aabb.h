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

    // https://gist.github.com/DomNomNom/46bb1ce47f68d255fd5d
    bool ray_intersection(glm::dvec3 ray_origin, glm::dvec3 ray_dir, double min_interval = -infinity, double max_interval = infinity) const;

    // Returns the index of the longest axis of the bounding box.
    int longest_axis() const {
        int x_size = abs(half_size.x);
        int y_size = abs(half_size.y);
        int z_size = abs(half_size.z);

        if (x_size > y_size)
            return x_size > z_size ? 0 : 2;
        else
            return y_size > z_size ? 1 : 2;
    }

    float axis_min(int axis) const {
        if (axis == 1) {
            return (center - half_size).y;
        } else
        if (axis == 2) {
            return (center - half_size).z;
        }

        return (center - half_size).x;
    }
};

AABB merge_aabbs(const AABB& AABB_0, const AABB& AABB_1);
