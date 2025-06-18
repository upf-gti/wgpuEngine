#pragma once

#include "graphics/primitives/primitive_mesh.h"

class QuadMesh : public PrimitiveMesh
{
    float width = 1.0f;
    float height = 1.0f;

    uint32_t subdivisions = 0u;

    bool flip_y = false;
    bool centered = true;

    void build_mesh() override;

public:

    QuadMesh(float width = 1.f, float height = 1.f, bool flip_y = false, bool centered = true, uint32_t subdivisions = 0u, const glm::vec3& color = { 1.f, 1.f, 1.f });

    void render_gui() override;

    float get_width() const { return width; }
    float get_height() const { return height; }
    float get_subdivisions() const { return subdivisions; }
    bool get_flip_y() const { return flip_y; }
    bool get_centered() const { return centered; }

    void set_width(float new_width);
    void set_height(float new_height);
    void set_subdivisions(uint32_t new_subdivisions);
    void set_size(const glm::vec2& size);
    void set_centered(bool new_centered);
    void set_flip_y(bool new_flip_y);
};
