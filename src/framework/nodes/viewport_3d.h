#pragma once

#include "mesh_instance_3d.h"
#include "graphics/material.h"
#include "graphics/surface.h"

class Node2D;

/*
*   This 3D Node allows to use a 2D UI in 3D
*/

class Viewport3D : public Node3D {

    bool active = true;

    Node2D* root = nullptr;

    glm::vec2 viewport_size = glm::vec2(0.0f);

public:

    Viewport3D(Node2D* root_2d);
	~Viewport3D();

	virtual void update(float delta_time) override;

    void set_viewport_size(const glm::vec2& new_size);
    void set_active(bool value) { active = value; }
};
