#pragma once

#include "includes.h"

#include <string>
#include <vector>

class Entity {

protected:
	std::string name;
	glm::mat4x4 model;

	Entity* parent = nullptr;
	std::vector<Entity*> children;

	bool model_dirty = false;

public:

	Entity() : model(1.0f) {};
	virtual ~Entity() {};

	void add_child(Entity* child);
	void remove_child(Entity* child);

	virtual void render();
	virtual void update(float delta_time);

	void translate(const glm::vec3& translation);
	void rotate(float angle, const glm::vec3& axis);
	void scale(glm::vec3 scale);

	// Some useful methods

	const glm::vec3& get_translation();
	const glm::mat4x4& get_global_matrix();
	void set_model(const glm::mat4x4& _model) { model = _model; model_dirty = true; }
};
