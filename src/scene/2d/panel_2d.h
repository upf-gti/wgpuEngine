#pragma once

#include "framework/colors.h"
#include "framework/ui/ui_utils.h"
#include "graphics/mesh.h"
#include "node_2d.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

class Text3D;

class Panel2D : public Node2D {
public:
    Color color = glm::vec4(0.0f);

    bool render_background = true;
    bool pressed_inside = false;
    bool on_hover = false;

    float last_release_time = 0.0f;
    float last_press_time = 0.0f;

    uint32_t parameter_flags = 0;

    Mesh* quad_mesh = nullptr;

    Panel2D() {}
    Panel2D(const std::string& name, const glm::vec2& p, const glm::vec2& s, uint32_t flags = 0u, const Color& c = colors::WHITE);
    Panel2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags, const Color& c = colors::WHITE);

    virtual ~Panel2D();

    sInputData get_input_data(bool ignore_focus = false) override;
    bool on_input(sInputData data) override;

    bool was_input_pressed();
    bool was_input_released();
    bool is_input_pressed();

    void update_scroll_view();
    bool on_pressed() override;

    void render() override;

    void disable_2d() override;
    void set_priority(uint8_t priority) override;
    void update_ui_data() override;
    void set_color(const Color& c);
    virtual void set_signal(const std::string& new_signal);

    uint32_t get_flags() { return parameter_flags; }
};
