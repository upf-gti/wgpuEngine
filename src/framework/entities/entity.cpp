#include "entity.h"


#include "spdlog/spdlog.h"

void Entity::add_child(Entity* child)
{
	if (child->parent) {
		spdlog::error("Child has already a parent, remove it first!");
		return;
	}

	// Checks if it's already a child
	auto it = std::find(children.begin(), children.end(), child);
	if (it != children.end()) {
        spdlog::error("Entity is already one of the children!");
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
        spdlog::error("Entity is not a child!!");
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
    if (!active || !process_children)
        return;

    for (Entity* child : children) {
        child->render();
    }
}

void Entity::update(float delta_time)
{
    if (!active || !process_children)
        return;

    for (Entity* child : children) {
        child->update(delta_time);
    }
}

void Entity::set_translation(const glm::vec3& translation)
{
	model = glm::translate(glm::mat4x4(1.f), translation);
}

const glm::vec3 Entity::get_local_translation()
{
    return model[3];
}

const glm::vec3 Entity::get_translation()
{
	return get_global_model()[3];
}

glm::mat4x4 Entity::get_global_model() const
{
	if (parent)
		return parent->get_global_model() * model;
	return model;
}

glm::mat4x4 Entity::get_rotation()
{
    glm::mat4x4 trans = model;
    glm::transpose(trans);

    glm::mat4x4 inv = model;
    glm::inverse(model);

    return trans * inv;
}

AABB Entity::get_aabb()
{
    AABB new_aabb = aabb;

    for (Entity* child : children) {

        AABB child_aabb = child->get_aabb();

        if (!new_aabb.initialized()) {
            new_aabb = child_aabb;
            continue;
        }

        if (child_aabb.initialized()) {
            new_aabb = merge_aabbs(new_aabb, child_aabb);
        }
    }

    return new_aabb;
}

