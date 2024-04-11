#pragma once

#include "light_3d.h"

class SpotLight3D : public Light3D {

    float inner_cone_angle = 0.0f;
    float outer_cone_angle = glm::pi<float>() / 4.0f;

public:

    SpotLight3D();
    ~SpotLight3D();

    void render_gui() override;

    sLightUniformData get_uniform_data() override;

    float get_inner_cone_angle() { return inner_cone_angle; };
    float get_outer_cone_angle() { return outer_cone_angle; };

    void set_inner_cone_angle(float value);
    void set_outer_cone_angle(float value);
};
