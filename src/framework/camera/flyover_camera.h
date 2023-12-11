#pragma once

#include "camera.h"

class FlyoverCamera : public Camera {

public:

    FlyoverCamera();

    void update(float delta_time) override;

    glm::vec3 inertial_speed = {};
};
