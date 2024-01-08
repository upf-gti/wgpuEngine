#include "flyover_camera.h"

#include "framework/input.h"
#include "framework/utils/utils.h"

#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

FlyoverCamera::FlyoverCamera() : Camera()
{
}

void FlyoverCamera::update(float delta_time)
{
    Camera::update(delta_time);

    float final_speed = speed;
    glm::vec3 move_dir = glm::vec3(0.0f, 0.0f, 0.0f);
    if (Input::is_key_pressed(GLFW_KEY_LEFT_SHIFT)) final_speed *= 10.0f;
    if (Input::is_key_pressed(GLFW_KEY_LEFT_CONTROL)) final_speed *= 0.1f;
    if (Input::is_key_pressed(GLFW_KEY_W) || Input::is_key_pressed(GLFW_KEY_UP))    move_dir += (glm::vec3(0.0f, 0.0f, -1.0f));
    if (Input::is_key_pressed(GLFW_KEY_S) || Input::is_key_pressed(GLFW_KEY_DOWN))  move_dir += (glm::vec3(0.0f, 0.0f, 1.0f));
    if (Input::is_key_pressed(GLFW_KEY_A) || Input::is_key_pressed(GLFW_KEY_LEFT))  move_dir += (glm::vec3(-1.0f, 0.0f, 0.0f));
    if (Input::is_key_pressed(GLFW_KEY_D) || Input::is_key_pressed(GLFW_KEY_RIGHT)) move_dir += (glm::vec3(1.0f, 0.0f, 0.0f));

    if (glm::length2(move_dir)) {
        move_dir = get_local_vector(move_dir);
        move_dir = normalize(move_dir) * final_speed * delta_time;
    }

    inertial_speed += move_dir;

    glm::vec3 new_forward = yaw_pitch_to_vector(delta_yaw, delta_pitch);
    glm::vec3 new_pos = eye + inertial_speed;

    look_at(new_pos, new_pos + new_forward, glm::vec3(0.0f, 1.0f, 0.0f));

    inertial_speed *= 0.9f;
}
