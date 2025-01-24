#pragma once

#include "camera.h"

class Node3D;

class Camera3D : public Camera {

public:

    Camera3D() = default;

    void apply_movement(const glm::vec2& movement);

    virtual void update(float delta_time);

    virtual void look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals = true);

    void look_at_entity(Node3D* entity);

protected:

    float delta_yaw = 0.0f;
    float delta_pitch = 0.0f;

    LerpedValue<float> delta_yaw_lerp;
    LerpedValue<float> delta_pitch_lerp;
    LerpedValue<glm::vec3> eye_lerp;
};
