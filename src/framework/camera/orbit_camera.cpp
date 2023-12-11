#include "orbit_camera.h"

#include "framework/input.h"
#include "utils.h"

#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

OrbitCamera::OrbitCamera() : Camera()
{
}

void OrbitCamera::update(float delta_time)
{
    Camera::update(delta_time);

    float distance = glm::length(eye - center);

    inertial_speed -= Input::get_mouse_wheel_delta() * 0.2f;

    distance += inertial_speed;

    if (distance < 0.001f) {
        distance = 0.001f;
    }

    glm::vec3 new_forward = glm::normalize(yaw_pitch_to_vector(delta_yaw, delta_pitch));

    look_at(center - new_forward * distance, center, glm::vec3(0.0f, 1.0f, 0.0f));

    inertial_speed *= 0.9f;
}
