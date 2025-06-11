#pragma once

#include "graphics/geometries/surface_geometry.h"

class CylinderGeometry : public SurfaceGeometry
{
    float radius = 1.0f;
    float height = 1.0f;

    uint32_t rings = 4u;
    uint32_t ring_segments = 64u;

    bool capped = true;

    void build_mesh() override;

public:

    CylinderGeometry(float radius = 1.0f, float height = 1.0f, uint32_t rings = 4u, uint32_t ring_segments = 64u, bool capped = true, const glm::vec3& color = { 1.f, 1.f, 1.f });

    void render_gui() override;

    float get_radius() const { return radius; }
    float get_height() const { return height; }
    uint32_t get_rings() const { return rings; }
    uint32_t get_ring_segments() const { return ring_segments; }
    bool get_capped() const { return capped; }

    void set_radius(float new_ring_radius);
    void set_height(float new_height);
    void set_rings(uint32_t new_rings);
    void set_ring_segments(uint32_t new_ring_segments);
    void set_capped(bool new_capped);
};
