#pragma once

#include "graphics/mesh.h"

class PrimitiveMesh : public Mesh
{
protected:

    Surface* surface = nullptr;

    glm::vec3 color = {1.0f, 1.0f, 1.0f};

    bool dirty = false;

    virtual void build_mesh();

public:

    PrimitiveMesh(const glm::vec3& new_color);

    void render_gui() override;

    void set_color(const glm::vec3& new_color);
};
