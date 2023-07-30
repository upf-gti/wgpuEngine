#pragma once

#include "entity.h"

class Mesh;
class Shader;

class EntityMesh : public Entity {

protected:

	Mesh*    mesh = nullptr;
	uint16_t instance_id = 0;

public:

	bool destroy_after_render = false;

	EntityMesh() : Entity() {};
	virtual ~EntityMesh() {};

	virtual void render() override;
	virtual void update(float delta_time) override;

	void  set_mesh(Mesh* mesh);
	Mesh* get_mesh() { return mesh; }
};
