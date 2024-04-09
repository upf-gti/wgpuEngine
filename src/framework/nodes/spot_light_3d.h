#pragma once

#include "light_3d.h"

class SpotLight3D : public Light3D {

    float range = 5.0f;
    float angle = 45.0f;

public:

    SpotLight3D();
    ~SpotLight3D();

    float get_range() { return range; };
    float get_angle() { return angle; };

    void set_range(float value);
    void set_angle(float value);
};
