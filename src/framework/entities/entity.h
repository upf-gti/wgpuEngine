#pragma once

#include "includes.h"

#include <string>
#include <vector>

class Entity {

public:

	Entity() {};
	virtual ~Entity() {};

	std::string name;
	glm::mat4x4 model = {};

	Entity* parent = nullptr;
	std::vector<Entity*> children;

	void addChild(Entity* child);
	void removeChild(Entity* child);

	virtual void render();
	virtual void update(float delta_time);

	// Some useful methods
	glm::mat4x4 getGlobalMatrix();
};
