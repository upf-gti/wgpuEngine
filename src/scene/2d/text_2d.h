#pragma once

#include "panel_2d.h"

class Text3D;

class Text2D : public Panel2D {
    static Text2D* selected;
    float text_scale = 1.0f;
    std::string text_string = "";

public:
    Text3D* text_entity = nullptr;

    Text2D() {}
    Text2D(const std::string& _text, const glm::vec2& pos, float scale = 16.f, uint32_t parameter_flags = 0, const Color& color = colors::BLACK);
    Text2D(const std::string& _text, float scale = 16.f, uint32_t parameter_flags = 0);

    void render() override;
    void update(float delta_time) override;
    bool on_input(sInputData data) override;
    void release() override;

    void set_text(const std::string& text);
    void disable_2d() override;
    void set_priority(uint8_t priority) override;
    void set_signal(const std::string& new_signal) override;

    static void select(Text2D* instance) { selected = instance; }
};
