#pragma once

#include "panel_2d.h"

class XRPanel : public Panel2D {
    bool is_button = false;
    bool fullscreen = false;

    glm::vec2 button_position = {};
    glm::vec2 button_size = {};

public:
    XRPanel(const std::string& name, const glm::vec2& p, const glm::vec2& s, uint32_t flags = 0, const Color& c = colors::WHITE);
    XRPanel(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags = 0, const Color& c = colors::WHITE);

    void update(float delta_time) override;

    sInputData get_input_data(bool ignore_focus = false) override;
    bool on_input(sInputData data) override;
    bool get_is_button() { return is_button; }

    void add_button(const std::string& signal, const std::string& texture_path, const glm::vec2& p, const glm::vec2& s, const Color& c = colors::WHITE);
    void make_as_button(const glm::vec2& bs, const glm::vec2& bp)
    {
        is_button = true;
        button_position = bp;
        button_size = bs;
    }
};
