#pragma once

#include "graphics/surface.h"

class SurfaceGeometry : public Surface
{
protected:

    glm::vec3 color = {1.0f, 1.0f, 1.0f};

    bool dirty = false;

    virtual void build_mesh() {}

public:

    SurfaceGeometry(const glm::vec3& new_color);

    void set_color(const glm::vec3& new_color);
};
