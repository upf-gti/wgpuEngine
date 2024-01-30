#pragma once

#include "camera.h"

class OrbitCamera : public Camera {

public:

    OrbitCamera();

    void update(float delta_time) override;

    void look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals = true) override;

    LerpedValue<float> distance_lerp;
    float distance = 0.0f;
};
