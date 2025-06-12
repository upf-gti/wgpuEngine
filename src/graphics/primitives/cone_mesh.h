#pragma once

#include "graphics/primitives/primitive_mesh.h"

class ConeMesh : public PrimitiveMesh
{
    float radius = 0.5f;
    float height = 2.0f;

    uint32_t segments = 64u;

    void build_mesh() override;

public:

    ConeMesh(float radius = 0.5f, float height = 2.0f, uint32_t segments = 64u, const glm::vec3& color = { 1.f, 1.f, 1.f });

    void render_gui() override;

    float get_radius() const { return radius; }
    float get_height() const { return height; }
    uint32_t get_segments() const { return segments; }

    void set_radius(float new_radius);
    void set_height(float new_height);
    void set_segments(uint32_t new_segments);
};
