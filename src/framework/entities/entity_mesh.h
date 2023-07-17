#pragma once

#include "entity.h"

class Mesh;

class EntityMesh : public Entity {

protected:

	Mesh* mesh = nullptr;

public:

	bool destroy_after_render = false;

	EntityMesh() : Entity() {};
	virtual ~EntityMesh() {};

	virtual void render() override;
	virtual void update(float delta_time) override;

	void  set_mesh(Mesh* mesh) { this->mesh = mesh; }
	Mesh* get_mesh() { return mesh; }
};
