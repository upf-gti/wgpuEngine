#pragma once

#include "mesh_instance_3d.h"

class Environment3D : public MeshInstance3D {

public:

    Environment3D();
	virtual ~Environment3D() {};

	void update(float delta_time) override;

    void set_texture(const std::string& texture_path);
};
