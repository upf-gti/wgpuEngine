#pragma once

#include "node_2d.h"
#include "mesh_instance_3d.h"
#include "graphics/material.h"
#include "graphics/surface.h"

class Viewport3D : public Node3D {

    Node2D* root = nullptr;

    glm::mat4x4 global_transform;

    MeshInstance3D* raycast_pointer = nullptr;

public:

    Viewport3D(Node2D* root_2d);
	virtual ~Viewport3D();

	virtual void render() override;
	virtual void update(float delta_time) override;
   
};
