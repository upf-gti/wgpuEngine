#include "camera_3d.h"

#include "framework/input.h"
#include "framework/nodes/node_3d.h"

void Camera3D::apply_movement(const glm::vec2& movement)
{
    {
        glm::vec2 delta = movement;

        delta.x = glm::clamp(delta.x, -150.0f, 150.0f);

        delta_yaw += delta.x * mouse_sensitivity;
        delta_pitch -= delta.y * mouse_sensitivity;
    }

    delta_yaw = clamp_rotation(delta_yaw);
    delta_pitch = clamp_rotation(delta_pitch);

    float max_offset = 0.25f;

    if (delta_pitch >= PI_2 - max_offset && delta_pitch < PI) {
        delta_pitch = PI_2 - 0.001f - max_offset;
    }

    if (delta_pitch > PI && delta_pitch <= 3.0f * PI_2 + max_offset) {
        delta_pitch = 3.0f * PI_2 + 0.001f + max_offset;
    }
}

void Camera3D::update(float delta_time)
{
    if (Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        apply_movement(Input::get_mouse_delta());
    }

    delta_pitch_lerp.value = smooth_damp_angle(delta_pitch_lerp.value, delta_pitch, &delta_pitch_lerp.velocity, 0.05f, 40.0f, delta_time);
    delta_yaw_lerp.value = smooth_damp_angle(delta_yaw_lerp.value, delta_yaw, &delta_yaw_lerp.velocity, 0.05f, 40.0f, delta_time);
}

void Camera3D::look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals)
{
    Camera::look_at(eye, center, up);

    if (reset_internals) {
        vector_to_yaw_pitch(glm::normalize(glm::vec3(center - eye)), &delta_yaw, &delta_pitch);
        delta_yaw_lerp.value = delta_yaw;
        delta_pitch_lerp.value = delta_pitch;
        eye_lerp.value = eye;
    }
}

void Camera3D::look_at_entity(Node3D* entity)
{
    if (!entity) {
        return;
    }

    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);

    AABB aabb = entity->get_aabb();

    float distance = 0.5f;

    if (aabb.initialized()) {
        distance = 3.0f * std::max(std::max(abs(aabb.half_size.x), abs(aabb.half_size.y)), abs(aabb.half_size.z));
    }

    look_at(aabb.center - distance * front, aabb.center, glm::vec3(0.0, 1.0, 0.0));
}
