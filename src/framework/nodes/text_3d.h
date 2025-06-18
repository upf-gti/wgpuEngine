#pragma once

#include <vector>
#include "mesh_instance_3d.h"
#include "framework/colors.h"
#include "graphics/font_common.h"

class Font;

class Text3D : public MeshInstance3D {

    sSurfaceData vertices;

    Font* font = nullptr;
    float font_scale = 1.f;

    std::string text = "";
    Color color;

    glm::vec2 box_size;
    bool wrap = true;
    bool is_2d = false;

    void append_char(glm::vec3 pos, Character& c);

public:

    Text3D(const std::string& text, const Color& color = colors::BLACK, bool is_2d = false, const glm::vec2& box_size = { 1, 1 }, bool wrap = false);
    virtual ~Text3D();

    virtual void update(float delta_time) override;

    void generate_mesh();

    int get_text_width(const std::string& text);
    int get_text_height(const std::string& text);
    const std::string& get_text() const { return text; }
    float get_scale() const { return font_scale; }
    bool get_wrap() const { return wrap; }

    void set_text(const std::string& p_text);
    void set_scale(float new_scale);
    void set_wrap(bool new_wrap);
    void set_is_2d(bool new_is_2d);
};
