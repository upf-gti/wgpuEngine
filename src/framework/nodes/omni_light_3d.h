#pragma once

#include "light_3d.h"

class OmniLight3D : public Light3D {

    float range = 5.0f;

public:

    OmniLight3D();
    ~OmniLight3D();

    float get_range() { return range; };

    void set_range(float value);
};
