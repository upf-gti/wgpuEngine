#pragma once

#include "entity.h"

class Mesh;
class Shader;

class EntityMesh : public Entity {

protected:

	Mesh*    mesh = nullptr;
	Shader*  shader = nullptr;
	uint16_t instance_id = 0;

	glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

public:

	EntityMesh() : Entity() {};
	virtual ~EntityMesh() {};

	virtual void render() override;
	virtual void update(float delta_time) override;

	void set_color(const glm::vec4& color) { this->color = color; }

	void  set_shader(Shader* shader) { this->shader = shader; }
	Shader* get_shader() { return shader; }

	void  set_mesh(Mesh* mesh);
	Mesh* get_mesh() { return mesh; }
};
