#pragma once

#include "node_2d.h"
#include "node_3d.h"
#include "graphics/material.h"
#include "graphics/surface.h"

class Viewport3D : public Node3D {

    Node2D* root = nullptr;

    float viewport_size = 1.0f;

public:

    Viewport3D(Node2D* root_2d);
	virtual ~Viewport3D();

	virtual void render() override;
	virtual void update(float delta_time) override;
   
};
