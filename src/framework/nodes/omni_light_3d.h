#pragma once

#include "light_3d.h"

class OmniLight3D : public Light3D {

    float range = -1.0f;

public:

    OmniLight3D();
    ~OmniLight3D();

    void render_gui() override;

    sLightUniformData get_uniform_data() override;

    float get_range() { return range; };

    void set_range(float value);
};
