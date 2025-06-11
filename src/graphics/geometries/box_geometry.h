#pragma once

#include "graphics/geometries/surface_geometry.h"

class BoxGeometry : public SurfaceGeometry
{
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;

    void build_mesh() override;

public:

    BoxGeometry(float width = 1.f, float height = 1.f, float depth = 1.f, const glm::vec3& color = { 1.f, 1.f, 1.f });

    void render_gui() override;

    float get_width() const { return width; }
    float get_height() const { return height; }
    float get_depth() const { return depth; }

    void set_width(float new_width);
    void set_height(float new_height);
    void set_depth(float new_depth);
    void set_size(const glm::vec3& size);
};
