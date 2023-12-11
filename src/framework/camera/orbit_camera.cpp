#include "orbit_camera.h"

#include "framework/input.h"
#include "utils.h"

#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

OrbitCamera::OrbitCamera() : Camera()
{
    speed = 0.05f;
}

void OrbitCamera::update(float delta_time)
{
    bool pan_enabled = Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_RIGHT) || Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_MIDDLE);

    glm::vec3 new_forward;

    if (!pan_enabled) {
        Camera::update(delta_time);

        new_forward = glm::normalize(yaw_pitch_to_vector(delta_yaw, delta_pitch));
    } else {
        glm::vec2 mouse_delta = Input::get_mouse_delta();

        new_forward = glm::normalize(yaw_pitch_to_vector(delta_yaw, delta_pitch));

        glm::vec3 right = glm::normalize(glm::cross(new_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(new_forward, right));

        right *= mouse_delta.x;
        up *= mouse_delta.y;

        eye += (right + up) * mouse_sensitivity;
        center += (right + up) * mouse_sensitivity;
    }

    float distance = glm::length(eye - center);

    float final_speed = speed;

    if (Input::is_key_pressed(GLFW_KEY_LEFT_SHIFT)) final_speed *= 10.0f;
    if (Input::is_key_pressed(GLFW_KEY_LEFT_CONTROL)) final_speed *= 0.1f;

    inertial_speed -= Input::get_mouse_wheel_delta() * final_speed;

    distance += inertial_speed;

    if (distance < 0.001f) {
        distance = 0.001f;
    }

    look_at(center - new_forward * distance, center, glm::vec3(0.0f, 1.0f, 0.0f));

    inertial_speed *= 0.9f;
}
