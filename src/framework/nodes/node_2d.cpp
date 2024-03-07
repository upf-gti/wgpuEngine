#include "node_2d.h"

#include "framework/input.h"

#include "spdlog/spdlog.h"

unsigned int Node2D::last_uid = 0;

std::map<std::string, Node2D*> Node2D::all_widgets;

Node2D::Node2D(const std::string& n, const glm::vec2& p, const glm::vec2& s) : size(s)
{
    uid = last_uid++;

    name = n;

    all_widgets[name] = this;

    set_translation(p);
}

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

    on_children_changed();
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

    on_children_changed();
}

void Node2D::render()
{
    Node::render();
}

void Node2D::update(float delta_time)
{
    Node::update(delta_time);
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

uint8_t Node2D::get_type() const
{
    return type;
}

glm::mat3x3 Node2D::get_rotation() const
{
    glm::mat4x4 trans = model;
    glm::transpose(trans);

    glm::mat4x4 inv = model;
    glm::inverse(model);

    return trans * inv;
}

glm::vec2 Node2D::get_size() const
{
    return size;
}

bool Node2D::is_hovered()
{
    glm::vec2 mouse_pos = Input::get_mouse_position();

    glm::vec2 min = get_translation();
    glm::vec2 max = min + size;

    return mouse_pos.x >= min.x && mouse_pos.y >= min.y && mouse_pos.x <= max.x && mouse_pos.y <= max.y;
}

Node2D* Node2D::get_widget_from_name(const std::string& name)
{
    if (all_widgets.count(name)) {
        return all_widgets[name];
    }
    return nullptr;
}
