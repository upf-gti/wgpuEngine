#pragma once

#include "graphics/primitives/primitive_mesh.h"

class CylinderMesh : public PrimitiveMesh
{
    float top_radius = 0.5f;
    float bottom_radius = 0.5f;
    float height = 2.0f;

    uint32_t rings = 4u;
    uint32_t ring_segments = 64u;

    bool cap_top = true;
    bool cap_bottom = true;

    void build_mesh() override;

public:

    CylinderMesh(float top_radius = 0.5f, float bottom_radius = 0.5f, float height = 2.0f, uint32_t rings = 4u, uint32_t ring_segments = 64u, bool cap_top = true, bool cap_bottom = true, const glm::vec3& color = { 1.f, 1.f, 1.f });

    void render_gui() override;

    float get_top_radius() const { return top_radius; }
    float get_bottom_radius() const { return bottom_radius; }
    float get_height() const { return height; }
    uint32_t get_rings() const { return rings; }
    uint32_t get_ring_segments() const { return ring_segments; }
    bool get_cap_top() const { return cap_top; }
    bool get_cap_bottom() const { return cap_bottom; }

    void set_top_radius(float new_top_radius);
    void set_bottom_radius(float new_bottom_radius);
    void set_height(float new_height);
    void set_rings(uint32_t new_rings);
    void set_ring_segments(uint32_t new_ring_segments);
    void set_cap_top(bool new_cap_top);
    void set_cap_bottom(bool new_cap_bottom);
};
