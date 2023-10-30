#pragma once

#include "camera.h"

class FlyoverCamera : public Camera {

public:
    void update(float delta_time) override;
};
