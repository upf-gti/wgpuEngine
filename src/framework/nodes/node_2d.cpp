#include "node_2d.h"
#include "spdlog/spdlog.h"

void Node2D::add_child(Node2D* child)
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

void Node2D::remove_child(Node2D* child)
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

void Node2D::translate(const glm::vec2& translation)
{
	model = glm::translate(model, translation);
}

void Node2D::rotate(float angle)
{
	model = glm::rotate(model, angle);
}

void Node2D::rotate(const glm::quat& q)
{
    model = model * glm::toMat3(q);
}

void Node2D::scale(glm::vec2 scale)
{
	model = glm::scale(model, scale);
}

void Node2D::set_model(const glm::mat3x3& _model)
{
    model = _model;
}

void Node2D::set_translation(const glm::vec2& translation)
{
	model = glm::translate(glm::mat3x3(1.f), translation);
}

const glm::vec2 Node2D::get_local_translation() const
{
    return model[2];
}

const glm::vec2 Node2D::get_translation() const
{
	return get_global_model()[2];
}

glm::mat3x3 Node2D::get_model() const
{
    return model;
}

glm::mat3x3 Node2D::get_global_model() const
{
	if (parent)
		return parent->get_global_model() * model;
	return model;
}

glm::mat3x3 Node2D::get_rotation() const
{
    glm::mat4x4 trans = model;
    glm::transpose(trans);

    glm::mat4x4 inv = model;
    glm::inverse(model);

    return trans * inv;
}