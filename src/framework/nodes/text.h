#pragma once

#include <vector>
#include "mesh_instance_3d.h"
#include "framework/colors.h"
#include "graphics/font_common.h"

class Font;

class TextEntity : public MeshInstance3D {

    std::vector<InterleavedData> vertices;

    Font* font = nullptr;

    float font_scale = 1.f;
    glm::vec2 box_size;
    bool wrap = true;

    eMaterialFlags flags;
    Color color;

    std::string text = "";

    void append_char(glm::vec3 pos, Character& c);

public:

    TextEntity(const std::string& _text, glm::vec2 _box_size = { 1, 1 }, bool _wrap = false);
    virtual ~TextEntity() {};

    virtual void update(float delta_time) override;

    /*
    *	Font Rendering
    */

    int get_text_width(const std::string& text);
    int get_text_height(const std::string& text);

    void generate_mesh(const Color& color, eMaterialFlags flags);
    eMaterialFlags get_flags() { return flags; }

    void set_text(const std::string& p_text) { text = p_text; generate_mesh(color, flags); };
    TextEntity* set_scale(float _scale) { font_scale = _scale; return this; }
};
