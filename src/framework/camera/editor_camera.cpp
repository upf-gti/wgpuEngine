#include "editor_camera.h"

#include "framework/input.h"

#include "glm/gtx/norm.hpp"

#include <spdlog/spdlog.h>

EditorCamera::EditorCamera() :
        Camera3D()
{
    speed = 0.005f;
}

void EditorCamera::update(float delta_time)
{
    custom_update(delta_time);

    check_update_mode();

    float final_speed = speed;

    if (Input::is_key_pressed(GLFW_KEY_LEFT_SHIFT)) {
        final_speed *= 10.0f;
    }
    if (Input::is_key_pressed(GLFW_KEY_LEFT_CONTROL)) {
        final_speed *= 0.1f;
    }

    glm::vec3 move_dir = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 new_forward = yaw_pitch_to_vector(delta_yaw_lerp.value, delta_pitch_lerp.value);

    glm::vec3 new_eye = eye;
    glm::vec3 new_center = center;

    float mouse_wheel_delta = Input::get_mouse_wheel_delta();

    distance -= Input::get_mouse_wheel_delta() * final_speed;

    if (distance < 0.001f) {
        distance = 0.001f;
    }

    distance_lerp.value = smooth_damp(distance_lerp.value, distance, &distance_lerp.velocity, 0.05f, 200.0f, delta_time);

    if (flyover_enabled) {
        if (Input::is_key_pressed(GLFW_KEY_W) || Input::is_key_pressed(GLFW_KEY_UP)) {
            move_dir += (glm::vec3(0.0f, 0.0f, -1.0f));
        }
        if (Input::is_key_pressed(GLFW_KEY_S) || Input::is_key_pressed(GLFW_KEY_DOWN)) {
            move_dir += (glm::vec3(0.0f, 0.0f, 1.0f));
        }
        if (Input::is_key_pressed(GLFW_KEY_A) || Input::is_key_pressed(GLFW_KEY_LEFT)) {
            move_dir += (glm::vec3(-1.0f, 0.0f, 0.0f));
        }
        if (Input::is_key_pressed(GLFW_KEY_D) || Input::is_key_pressed(GLFW_KEY_RIGHT)) {
            move_dir += (glm::vec3(1.0f, 0.0f, 0.0f));
        }

        if (glm::length2(move_dir)) {
            move_dir = get_local_vector(move_dir);
            move_dir = normalize(move_dir) * final_speed;
        }

        move_dir_lerp.value = smooth_damp(move_dir_lerp.value, move_dir, &move_dir_lerp.velocity, 0.05f, 500.0f, delta_time);

        new_eye = new_eye + move_dir_lerp.value * delta_time;
        new_center = new_eye + new_forward * distance_lerp.value;
    } else if (orbit_enabled || pan_enabled || mouse_wheel_delta != 0.0f) {
        if (pan_enabled && mouse_wheel_delta == 0.0f) {
            glm::vec2 mouse_delta = Input::get_mouse_delta();

            glm::vec3 right = glm::normalize(glm::cross(new_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 up = glm::normalize(glm::cross(new_forward, right));

            right *= mouse_delta.x;
            up *= mouse_delta.y;

            new_center += (right + up) * mouse_sensitivity;
        }

        new_eye = new_center - new_forward * distance_lerp.value;

        move_dir_lerp.value = smooth_damp(move_dir_lerp.value, glm::vec3(0.0f), &move_dir_lerp.velocity, 0.05f, 500.0f, delta_time);
    }

    look_at(new_eye, new_center, glm::vec3(0.0f, 1.0f, 0.0f), false);
}

void EditorCamera::look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals)
{
    Camera3D::look_at(eye, center, up, reset_internals);

    if (reset_internals) {
        distance = glm::length(eye - center);
        distance_lerp.value = distance;
    }
}

void EditorCamera::custom_update(float delta_time)
{
    if ((Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_MIDDLE) && !Input::is_key_pressed(GLFW_KEY_LEFT_SHIFT)) || Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        apply_movement(Input::get_mouse_delta());
    }

    delta_pitch_lerp.value = smooth_damp_angle(delta_pitch_lerp.value, delta_pitch, &delta_pitch_lerp.velocity, 0.05f, 40.0f, delta_time);
    delta_yaw_lerp.value = smooth_damp_angle(delta_yaw_lerp.value, delta_yaw, &delta_yaw_lerp.velocity, 0.05f, 40.0f, delta_time);
}

void EditorCamera::check_update_mode()
{
    if (Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        flyover_enabled = true;
        orbit_enabled = false;
        pan_enabled = false;
    } else {
        if (Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_MIDDLE) && Input::is_key_pressed(GLFW_KEY_LEFT_SHIFT)) {
            pan_enabled = true;
            orbit_enabled = false;
            flyover_enabled = false;

        } else if (Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_MIDDLE) || Input::get_mouse_wheel_delta() != 0.0f) {
            pan_enabled = false;
            orbit_enabled = true;
            flyover_enabled = false;
        } else {
            pan_enabled = false;
        }
    }
}
