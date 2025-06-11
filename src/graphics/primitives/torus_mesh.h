#pragma once

#include "graphics/primitives/primitive_mesh.h"

class TorusMesh : public PrimitiveMesh
{
    float ring_radius = 1.f;
    float tube_radius = 0.2f;

    uint32_t rings = 64u;
    uint32_t ring_segments = 32u;

    void build_mesh() override;

public:

    TorusMesh(float ring_radius = 1.f, float tube_radius = 0.2f, uint32_t rings = 64u, uint32_t ring_segments = 32u, const glm::vec3& color = { 1.f, 1.f, 1.f });

    //void render_gui() override;

    float get_ring_radius() const { return ring_radius; }
    float get_tube_radius() const { return tube_radius; }
    uint32_t get_rings() const { return rings; }
    uint32_t get_ring_segments() const { return ring_segments; }

    void set_ring_radius(float new_ring_radius);
    void set_tube_radius(float new_tube_radius);
    void set_rings(uint32_t new_rings);
    void set_ring_segments(uint32_t new_ring_segments);
};
