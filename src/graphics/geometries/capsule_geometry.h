#pragma once

#include "graphics/geometries/surface_geometry.h"

class CapsuleGeometry : public SurfaceGeometry
{
    float radius = 1.0f;
    float height = 1.0f;

    uint32_t rings = 64u;
    uint32_t ring_segments = 32u;

    void build_mesh() override;

public:

    CapsuleGeometry(float radius = 1.0f, float height = 1.0f, uint32_t rings = 8u, uint32_t ring_segments = 64u, const glm::vec3& color = { 1.f, 1.f, 1.f });

    void render_gui() override;

    float get_radius() const { return radius; }
    float get_height() const { return height; }
    uint32_t get_rings() const { return rings; }
    uint32_t get_ring_segments() const { return ring_segments; }

    void set_radius(float new_ring_radius);
    void set_height(float new_height);
    void set_rings(uint32_t new_rings);
    void set_ring_segments(uint32_t new_ring_segments);
};
