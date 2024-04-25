#pragma once

#include "node_3d.h"

#include "graphics/uniforms_structs.h"

enum LightType {
    LIGHT_UNDEFINED,
    LIGHT_DIRECTIONAL,
    LIGHT_OMNI,
    LIGHT_SPOT
};

class Light3D : public Node3D {

protected:

    LightType   type = LIGHT_UNDEFINED;

    float       intensity = 1.0f;
    glm::vec3   color = { 1.0f, 1.0f, 1.0f };
    float       range = -1.0f;

    // Fading

    bool fading_enabled = false;
    float fade_begin = 10.0f;
    float fade_length = 1.0f;

    // Shadows
    bool cast_shadows = false;
    float shadow_bias = 0.001f;

public:

    Light3D();
    ~Light3D();

    void render_gui() override;

    virtual sLightUniformData get_uniform_data() = 0;

    LightType get_type() { return type; };


    float get_intensity() { return intensity; };
    const glm::vec3& get_color() { return color; };
    bool get_fading_enabled() { return fading_enabled; };
    bool get_cast_shadows() { return cast_shadows; };
    float get_range() { return range; };

    void set_color(glm::vec3 color);
    void set_intensity(float value);
    void set_range(float value);
    void set_fading_enabled(bool value);
    void set_cast_shadows(bool value);
};
