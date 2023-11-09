#pragma once

#include "includes.h"

#include <string>
#include <vector>

class Entity {

protected:
	std::string name;
	glm::mat4x4 model = glm::mat4x4(1.0f);

	Entity* parent = nullptr;
	std::vector<Entity*> children;

public:

	Entity() : model(1.0f) {};
	virtual ~Entity() {};

	void add_child(Entity* child);
	void remove_child(Entity* child);

	virtual void render();
	virtual void update(float delta_time);

	void translate(const glm::vec3& translation);
	void rotate(float angle, const glm::vec3& axis);
    void rotate(const glm::quat& q);
	void scale(glm::vec3 scale);

	// Some useful methods

	void set_translation(const glm::vec3& translation);
	const glm::vec3& get_local_translation();
    const glm::vec3 get_translation();
	glm::mat4x4 get_global_model();
	void set_model(const glm::mat4x4& _model) { model = _model; }
    glm::mat4x4 get_model() { return model; }
    const std::vector<Entity*>& get_children() const { return children; }
    Entity* get_parent() { return parent; }
};
