#pragma once

#include "light_3d.h"

class OmniLight3D : public Light3D {

public:

    OmniLight3D();
    ~OmniLight3D();

    void render_gui() override;

    sLightUniformData get_uniform_data() override;
};
