#pragma once

#include "includes.h"

namespace Intersection {
	inline bool ray_AABB(const glm::vec3& ray_origin,
						 const glm::vec3& ray_direction,
						 const glm::vec3& box_origin,
						 const glm::vec3& box_size,
						 float* collision_distance) {
		// Using the slabs method
		const glm::vec3 inv_ray_dir = 1.0f / ray_direction;
		const glm::vec3 box_halfsize = box_size / 2.0f;

		const glm::vec3 box_min = box_origin - box_halfsize;
		const glm::vec3 box_max = box_origin + box_halfsize;

		// Slab intersection tests
		const glm::vec3 t1 = (box_min - ray_origin) * inv_ray_dir;
		const glm::vec3 t2 = (box_max - ray_origin) * inv_ray_dir;

		// Intersection test result
		const glm::vec3 tmin = glm::min(t1, t2);
		const float t_min = glm::max(tmin.x, glm::max(tmin.y, tmin.z));
		const glm::vec3 tmax = glm::max(t1, t2);
		const float t_max = glm::min(tmax.x, glm::min(tmax.y, tmax.z));

		// NOTE: in the case the ray origin is inside AABB, take t_max
		*collision_distance = t_min;

		return t_min < t_max;
	}

	inline bool ray_OBB(const glm::vec3& ray_origin,
						const glm::vec3& ray_direction,
						const glm::vec3& box_origin,
						const glm::vec3& box_size,
						const glm::quat& box_rotation,
						float* collision_distance) {
		// Translate the Ray to the OBB space, and procede like an AABB
		const glm::mat3x3 rotate_to_OBB = glm::mat3_cast(glm::inverse(box_rotation));

		return ray_AABB(rotate_to_OBB * ray_origin,
						rotate_to_OBB * ray_direction,
						rotate_to_OBB * box_origin,
						box_size,
						collision_distance);
	}
}