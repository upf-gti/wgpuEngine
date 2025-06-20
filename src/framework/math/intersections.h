#pragma once

#include "includes.h"
#include "spdlog/spdlog.h"

#include "aabb.h"

#include "glm/gtx/quaternion.hpp"

#define EPSILON 0.0001f

namespace intersection {

    inline bool ray_plane(const glm::vec3& ray_origin,
        const glm::vec3& ray_direction,
        const glm::vec3& plane_origin,
        const glm::vec3& plane_orientation,
        float* collision_distance) {

        const float facing = glm::dot(ray_direction, plane_orientation);

        if (glm::abs(facing) > EPSILON) {
            const glm::vec3 p = plane_origin - ray_origin;
            const float distance = glm::dot(p, plane_orientation) / facing;
            *collision_distance = distance;
            return distance >= 0;
        }

        return false;
    }

    inline bool ray_quad(const glm::vec3& ray_origin,
        const glm::vec3& ray_direction,
        const glm::vec3& quad_origin,
        const glm::vec2& quad_size,
        const glm::quat& quad_rotation,
        glm::vec3& intersection,
        glm::vec3& local_intersection,
        float* collision_distance,
        bool centered = true) {

        // Assumtion: the original quad orientation is (0,1,0)
        const glm::vec3 quad_orientation = quad_rotation * glm::vec3(0.f, 0.f, 1.f);

        // First, plane intersection
        float plane_intersection_distance;

        if (!ray_plane(ray_origin,
            ray_direction,
            quad_origin,
            quad_orientation,
            &plane_intersection_distance)) {
            return false;
        }

        // Second, is the intesected point inside a quat?
        const glm::quat rotate_to_quad_local = glm::inverse(quad_rotation);

        glm::vec3 intersection_point = (ray_origin + ray_direction * plane_intersection_distance);
        intersection = intersection_point;
        *collision_distance = plane_intersection_distance;

        // To local position
        intersection_point -= quad_origin;
        intersection_point = rotate_to_quad_local * intersection_point;
        local_intersection = intersection_point;

        if (centered) {
            if (quad_size.x > intersection_point.x && -quad_size.x < intersection_point.x)
                if (quad_size.y > intersection_point.y && -quad_size.y < intersection_point.y)
                    return true;
        }
        else {
            if (quad_size.x > intersection_point.x && 0.0f < intersection_point.x)
                if (quad_size.y > intersection_point.y && 0.0f < intersection_point.y)
                    return true;
        }

        return false;
    }

    inline bool ray_curved_quad(const glm::vec3& ray_origin,
        const glm::vec3& ray_direction,
        const glm::vec3& quad_origin,
        const glm::vec2& quad_size,
        const glm::quat& quad_rotation,
        uint32_t subdivisions,
        float curvature_factor,
        glm::vec3& intersection,
        glm::vec3& local_intersection,
        float* collision_distance,
        bool centered = true) {

        const glm::vec2& step = glm::vec2(quad_size.x / subdivisions, quad_size.y / subdivisions);
        const glm::vec3& centered_quad_origin = quad_origin - glm::vec3((subdivisions - 1u) * step.x, (subdivisions - 1u) * step.y, 0.0f);

        // Generate vertices with positions and UVs
        for (int i = 0; i < subdivisions; ++i) {
            for (int j = 0; j < subdivisions; ++j) {

                glm::vec3 new_quad_origin = centered_quad_origin + glm::vec3(j * step.x * 2.f, i * step.y * 2.0f, 0.0f);
                glm::quat rot = { 0.0f, 0.0f, 0.0f, 1.0f };

                float uv0_x = (static_cast<float>(j) / subdivisions) * 2.0f - 1.0f;
                float uv1_x = (static_cast<float>(j + 1u) / subdivisions) * 2.0f - 1.0f;

                float z0 = curvature_factor * (1.0f - (fabsf(uv0_x * uv0_x) * 0.5f + 0.5f));
                float z1 = curvature_factor * (1.0f - (fabsf(uv1_x * uv1_x) * 0.5f + 0.5f));

                if (fabsf(z0 - z1) > 0.001f) {

                    glm::vec3 quad_corner;
                    float sign = z1 > z0 ? 1.0f : -1.0;

                    // opens.. so get the min
                    if (z1 > z0) {
                        quad_corner = new_quad_origin - glm::vec3(step, z0);
                    }
                    // closes.. get max
                    else {
                        quad_corner = new_quad_origin + glm::vec3(step, -z1);
                    }

                    const glm::vec2& p0 = { quad_corner.x, -z0 };
                    const glm::vec2& p1 = { quad_corner.x + step.x * sign, -z0 };
                    const glm::vec2& p2 = { quad_corner.x + step.x * sign, -z1 };

                    glm::vec2 d1 = p1 - p0;
                    glm::vec2 d2 = p2 - p0;

                    if (glm::length(d1) > 0.0f) {
                        d1 = glm::normalize(d1);
                    }

                    if (glm::length(d2) > 0.0f) {
                        d2 = glm::normalize(d2);
                    }

                    float angle = glm::acos(glm::clamp(glm::dot(d1, d2), -1.0f, 1.0f)) * 0.5f;

                    glm::mat4x4 m = glm::translate(glm::mat4x4(1.0f), quad_corner);
                    m = glm::rotate(m, angle * sign, glm::vec3(0.0f, 1.0f, 0.0f));

                    rot = glm::toQuat(m);

                    new_quad_origin = m * glm::vec4(step * sign, 0.0f, 1.0f);
                }
                else {
                    new_quad_origin.z -= z0;
                }

                if (ray_quad(
                    ray_origin,
                    ray_direction,
                    new_quad_origin,
                    step,
                    quad_rotation * rot,
                    intersection,
                    local_intersection,
                    collision_distance,
                    centered
                )) {
                    return true;
                }
            }
        }

        return false;
    }

    inline bool ray_circle(const glm::vec3& ray_origin,
        const glm::vec3& ray_direction,
        const glm::vec3& circle_origin,
        const float circle_radius,
        const glm::quat& circle_rotation,
        float* collision_distance) {

        // Assumtion: the original circle orientation is (0,1,0)
        const glm::vec3 circle_orientation = circle_rotation * glm::vec3(0.f, 1.f, 0.f);

        // First, plane intersection
        float plane_intersection_distance;

        if (!ray_plane(ray_origin,
            ray_direction,
            circle_origin,
            circle_orientation,
            &plane_intersection_distance)) {
            return false;
        }

        // Second, is the intesected point inside the circle?
        const glm::vec3 intersection_point = (ray_origin + ray_direction * plane_intersection_distance);
        *collision_distance = plane_intersection_distance;
        return glm::length(intersection_point - circle_origin) < circle_radius;
    }

    inline bool ray_sphere(const glm::vec3& ray_origin,
        const glm::vec3& ray_direction,
        const glm::vec3& sphere_center,
        float sphere_radius,
        float* collision_distance) {

        const glm::vec3 origin_to_center = sphere_center - ray_origin;
        const double a = glm::length2(ray_direction);
        const double h = glm::dot(ray_direction, origin_to_center);
        const double c = glm::length2(origin_to_center) - sphere_radius * sphere_radius;

        const double discriminant = h * h - a * c;

        if (discriminant < 0) {
            return false;
        }

        *collision_distance = (h - glm::sqrt(discriminant)) / a;

        return true;
    }

    inline bool ray_AABB(const glm::vec3& ray_origin,
        const glm::vec3& ray_direction,
        const glm::vec3& box_origin,
        const glm::vec3& box_halfsize,
        float* collision_distance) {

        // Using the slabs method
        const glm::vec3 inv_ray_dir = 1.0f / ray_direction;

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

        // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
        // if tmin > tmax, ray doesn't intersect AABB
        if (t_max < 0 || t_min > t_max)
        {
            *collision_distance = t_max;
            return false;
        }

        *collision_distance = t_min;
        return true;
    }

    inline bool ray_OBB(const glm::vec3& ray_origin,
        const glm::vec3& ray_direction,
        const glm::vec3& box_origin,
        const glm::vec3& box_halfsize,
        const glm::quat& box_rotation,
        float* collision_distance) {

        // Translate the Ray to the OBB space, and procede like an AABB
        const glm::quat rotate_to_OBB = glm::inverse(box_rotation);

        return ray_AABB(rotate_to_OBB * ray_origin,
            rotate_to_OBB * ray_direction,
            rotate_to_OBB * box_origin,
            box_halfsize,
            collision_distance);
    }

    inline bool point_AABB(const glm::vec3& point,
        const glm::vec3& box_origin,
        const glm::vec3& box_size) {

        const glm::vec3 box_halfsize = box_size / 2.0f;
        const glm::vec3 box_min = box_origin - box_halfsize;

        const glm::vec3 t = point - box_min;

        return box_size.x > t.x && box_size.y > t.y && box_size.z > t.z && t.x > 0.0f && t.y > 0.0f && t.z > 0.0f;
    }

    inline bool point_plane(const glm::vec3& point,
        const glm::vec3& plane_origin,
        const glm::vec3& plane_normal,
        float* distance,
        const float epsilon = EPSILON) {
        float dist_to_plane = glm::dot(plane_normal, point - plane_origin);
        *distance = dist_to_plane - epsilon;

        return glm::abs(dist_to_plane) < epsilon;
    }

    inline bool point_circle(const glm::vec3& point,
        const glm::vec3& circle_origin,
        const glm::vec3& circle_normal,
        const float      circle_radius) {
        float distance;

        if (!point_plane(point, circle_origin, circle_normal, &distance, 0.02f)) {
            return false;
        }
        const glm::vec3 point_in_plane = point - circle_normal * distance;

        return glm::abs(glm::length(point_in_plane - circle_origin)) < circle_radius;
    }

    inline bool point_circle_ring(const glm::vec3& point,
        const glm::vec3& circle_origin,
        const glm::vec3& circle_normal,
        const float      circle_radius,
        const float      ring_size) {
        float distance;

        if (!point_plane(point, circle_origin, circle_normal, &distance, 0.02f)) {
            return false;
        }
        const glm::vec3 point_in_plane = point - circle_normal * distance;

        const float dist = glm::abs(glm::length(point_in_plane - circle_origin));

        return dist <= circle_radius && dist >= (circle_radius - ring_size);
    }

    inline bool point_sphere(const glm::vec3& point,
        const glm::vec3& sphere_center,
        const float      radius) {
        return glm::abs(glm::length(point - sphere_center)) <= radius;
    }

    inline bool AABB_AABB_min_max(const AABB& box1, const AABB& box2)
    {
        glm::vec3 box1_min = box1.center - box1.half_size;
        glm::vec3 box1_max = box1.center + box1.half_size;
        glm::vec3 box2_min = box2.center - box2.half_size;
        glm::vec3 box2_max = box2.center + box2.half_size;

        return glm::all(glm::lessThanEqual(box1_min, box2_max)) && glm::all(glm::greaterThanEqual(box1_max, box2_min));
    }
}
