#include "entity.h"

#include <glm/gtx/quaternion.hpp>
#include <iostream>

void Entity::add_child(Entity* child)
{
	if (child->parent) {
		std::cerr << "Child has already a parent, remove it first!" << std::endl;
		return;
	}

	// Checks if it's already a child
	auto it = std::find(children.begin(), children.end(), child);
	if (it != children.end()) {
		std::cerr << "Entity is already one of the children!" << std::endl;
		return;
	}

	child->parent = this;
	children.push_back(child);
}

void Entity::remove_child(Entity* child)
{
	// Checks if it's a child
	auto it = std::find(children.begin(), children.end(), child);
	if (it == children.end()) {
		std::cerr << "Entity is not a child!!" << std::endl;
		return;
	}

	children.erase(it);
	child->parent = nullptr;
}

void Entity::translate(const glm::vec3& translation)
{
	model = glm::translate(model, translation);
}

void Entity::rotate(float angle, const glm::vec3& axis)
{
	model = glm::rotate(model, angle, axis);
}

void Entity::rotate(const glm::quat& q) {
    model = model * glm::toMat4(q);
}

void Entity::scale(glm::vec3 scale)
{
	model = glm::scale(model, scale);
}

void Entity::render()
{
    for (Entity* child : children) {
        child->render();
    }
}

void Entity::update(float delta_time)
{
    for (Entity* child : children) {
        child->update(delta_time);
    }
}

void Entity::set_translation(const glm::vec3& translation)
{
	model = glm::translate(glm::mat4x4(1.f), translation);
}

const glm::vec3& Entity::get_local_translation()
{
    return model[3];
}

const glm::vec3 Entity::get_translation()
{
	return get_global_model()[3];
}

glm::mat4x4 Entity::get_global_model()
{
	if (parent)
		return model * parent->get_global_model();
	return model;
}
