#pragma once

#include "panel_2d.h"

class ColorPicker2D : public Panel2D {
public:
    float ring_thickness = 0.25f;

    bool changing_hue = false;
    bool changing_sv = false;

    ColorPicker2D() {}
    ColorPicker2D(const std::string& sg, const Color& c);
    ColorPicker2D(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c, uint32_t flags = 0);

    void update(float delta_time) override;
    bool on_input(sInputData data) override;

    glm::vec2 uv_to_saturation_value(const glm::vec2& uvs);
};
