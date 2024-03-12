#pragma once

#include "node_2d.h"
#include "mesh_instance_3d.h"
#include "graphics/material.h"
#include "graphics/surface.h"

class Viewport3D : public Node3D {

    Node2D* root = nullptr;

    MeshInstance3D* raycast_pointer = nullptr;

    glm::vec2 viewport_size = glm::vec2(0.0f);

public:

    Viewport3D(Node2D* root_2d);
	~Viewport3D();

	virtual void render() override;
	virtual void update(float delta_time) override;

    void set_viewport_size(const glm::vec2& new_size);
};
