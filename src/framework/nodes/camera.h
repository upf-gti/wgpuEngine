#pragma once

#include "framework/nodes/node_3d.h"
#include "framework/camera/camera_3d.h"

class EntityCamera : public Camera3D, public Node3D {

protected:


public:

    EntityCamera();
	virtual ~EntityCamera() {};

	// void update(float delta_time) override;
};
