#include "node_3d.h"
#include "spdlog/spdlog.h"

Node3D::Node3D() : model(1.0f)
{
    properties["translation"] = &transform.position;
    properties["rotation"] = &transform.rotation;
    properties["scale"] = &transform.scale;
}

void Node3D::add_child(Node3D* child)
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

void Node3D::remove_child(Node3D* child)
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

void Node3D::render()
{
    Node::render();
}

void Node3D::update(float delta_time)
{
    Node::update(delta_time);
}

void Node3D::translate(const glm::vec3& translation)
{
	model = glm::translate(model, translation);
}

void Node3D::rotate(float angle, const glm::vec3& axis)
{
	model = glm::rotate(model, angle, axis);
}

void Node3D::rotate(const glm::quat& q)
{
    model = model * glm::toMat4(q);
}

void Node3D::scale(glm::vec3 scale)
{
	model = glm::scale(model, scale);
}

void Node3D::set_translation(const glm::vec3& translation)
{
	model = glm::translate(glm::mat4x4(1.f), translation);
}

const glm::vec3 Node3D::get_local_translation() const
{
    return model[3];
}

const glm::vec3 Node3D::get_translation() const
{
    return get_global_model()[3];
}

glm::mat4x4 Node3D::get_global_model() const
{
    if (parent)
        return parent->get_global_model() * model;
    return model;
}

glm::mat4x4 Node3D::get_model() const
{
    return model;
}

glm::mat4x4 Node3D::get_rotation() const
{
    glm::mat4x4 trans = model;
    glm::transpose(trans);

    glm::mat4x4 inv = model;
    glm::inverse(model);

    return trans * inv;
}

Node3D* Node3D::get_parent() const
{
    return parent;
}

