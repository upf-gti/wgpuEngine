#pragma once

#include "graphics/surface.h"

class TorusGeometry : public Surface
{
    float ring_radius = 1.f;
    float tube_radius = 0.2f;

    uint32_t rings = 64u;
    uint32_t ring_segments = 32u;

    glm::vec3 color = {1.0f, 1.0f, 1.0f};

    bool dirty = false;

public:

    TorusGeometry(float ring_radius = 1.f, float tube_radius = 0.2f, uint32_t rings = 64u, uint32_t ring_segments = 32u, const glm::vec3& color = { 1.f, 1.f, 1.f });

    void render_gui() override;
   
};
