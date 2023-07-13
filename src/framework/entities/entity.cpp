#include "entity.h"

#include <iostream>

void Entity::addChild(Entity* child)
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

void Entity::removeChild(Entity* child)
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

void Entity::render()
{
}

void Entity::update(float delta_time)
{
}

glm::mat4x4 Entity::getGlobalMatrix()
{
	if (parent)
		return model * parent->getGlobalMatrix();
	return model;
}
