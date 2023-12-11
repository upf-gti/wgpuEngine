#pragma once

#include "camera.h"

class OrbitCamera : public Camera {

public:

    OrbitCamera();

    void update(float delta_time) override;

    float inertial_speed = 0.0f;
};
