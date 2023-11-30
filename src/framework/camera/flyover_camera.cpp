#include "flyover_camera.h"

#include "framework/input.h"

#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

FlyoverCamera::FlyoverCamera() : Camera()
{
}

void FlyoverCamera::update(float delta_time)
{
    if (Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        glm::vec2 mouse_delta = Input::get_mouse_delta();

        glm::vec3 front = normalize(center - eye);

        front = rotate(front, mouse_delta.y * mouse_sensitivity, get_local_vector(glm::vec3(1.0f, 0.0f, 0.0f)));
        front = rotate(front, mouse_delta.x * mouse_sensitivity, glm::vec3(0.0f, 1.0f, 0.0f));

        center = eye + front;
        update_view_matrix();
    }

    float final_speed = speed;
    glm::vec3 move_dir = glm::vec3(0.0f, 0.0f, 0.0f);
    if (Input::is_key_pressed(GLFW_KEY_LEFT_SHIFT)) final_speed *= 10.0f;
    if (Input::is_key_pressed(GLFW_KEY_LEFT_CONTROL)) final_speed *= 0.1f;
    if (Input::is_key_pressed(GLFW_KEY_W) || Input::is_key_pressed(GLFW_KEY_UP))    move_dir += (glm::vec3(0.0f, 0.0f, -1.0f));
    if (Input::is_key_pressed(GLFW_KEY_S) || Input::is_key_pressed(GLFW_KEY_DOWN))  move_dir += (glm::vec3(0.0f, 0.0f, 1.0f));
    if (Input::is_key_pressed(GLFW_KEY_A) || Input::is_key_pressed(GLFW_KEY_LEFT))  move_dir += (glm::vec3(-1.0f, 0.0f, 0.0f));
    if (Input::is_key_pressed(GLFW_KEY_D) || Input::is_key_pressed(GLFW_KEY_RIGHT)) move_dir += (glm::vec3(1.0f, 0.0f, 0.0f));

    if (!glm::length2(move_dir)) {
        return;
    }

    move_dir = get_local_vector(move_dir);

    move_dir = normalize(move_dir) * final_speed * delta_time;

    center += move_dir;
    eye += move_dir;

    update_view_matrix();
}
