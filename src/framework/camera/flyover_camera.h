#pragma once

#include "camera_3d.h"

class FlyoverCamera : public Camera3D {

public:

    FlyoverCamera();

    void update(float delta_time) override;
};
