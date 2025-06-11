#pragma once

#include "graphics/primitives/primitive_mesh.h"

class CapsuleMesh : public PrimitiveMesh
{
    float radius = 0.5f;
    float height = 2.0f;

    uint32_t rings = 8u;
    uint32_t ring_segments = 64u;

    void build_mesh() override;

public:

    CapsuleMesh(float radius = 0.5f, float height = 2.0f, uint32_t rings = 8u, uint32_t ring_segments = 64u, const glm::vec3& color = { 1.f, 1.f, 1.f });

    //void render_gui() override;

    float get_radius() const { return radius; }
    float get_height() const { return height; }
    uint32_t get_rings() const { return rings; }
    uint32_t get_ring_segments() const { return ring_segments; }

    void set_radius(float new_ring_radius);
    void set_height(float new_height);
    void set_rings(uint32_t new_rings);
    void set_ring_segments(uint32_t new_ring_segments);
};
