#pragma once

#include "entity.h"

#include "graphics/mesh.h"

class EntityMesh : public Entity {

	Mesh mesh;

public:

	EntityMesh() : Entity() {};
	virtual ~EntityMesh() {};

	virtual void render() override;
	virtual void update(float delta_time) override;

	Mesh* get_mesh() { return &mesh; }
};
