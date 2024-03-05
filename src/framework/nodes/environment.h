#pragma once

#include "mesh_instance_3d.h"

class Environment3D : public MeshInstance3D {

protected:

    std::vector<Surface*> surfaces;
    std::unordered_map<Surface*, Material> material_overrides;

public:

    Environment3D();
	virtual ~Environment3D() {};

	void render() override;
	void update(float delta_time) override;

    void set_texture(const std::string& texture_path);
};
