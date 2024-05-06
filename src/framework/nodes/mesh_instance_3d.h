#pragma once

#include "node_3d.h"
#include "graphics/mesh_instance.h"

class Shader;
class Texture;

class MeshInstance3D : public MeshInstance, public Node3D {

public:

    MeshInstance3D();
	virtual ~MeshInstance3D();

    AABB get_surface_world_aabb(int surface_idx);

	virtual void render() override;
	virtual void update(float delta_time) override;
};
