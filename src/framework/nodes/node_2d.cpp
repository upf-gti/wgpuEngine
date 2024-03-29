#include "node_2d.h"

#include "framework/input.h"
#include "framework/utils/intersections.h"
#include "graphics/renderer.h"

#include "spdlog/spdlog.h"

#include <algorithm>

unsigned int Node2D::last_uid = 0;
bool Node2D::propagate_event = true;

std::map<std::string, Node2D*> Node2D::all_widgets;
std::vector<std::pair<Node2D*, sInputData>> Node2D::frame_inputs;

Node2D::Node2D(const std::string& n, const glm::vec2& p, const glm::vec2& s) : size(s)
{
    type = NodeType::NODE_2D;

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

void Node2D::on_children_changed()
{
    if (parent) {
        parent->on_children_changed();
    }
}

void Node2D::render()
{
    if (!visibility)
        return;

    Node::render();
}

void Node2D::update(float delta_time)
{
    if (!visibility)
        return;

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

void Node2D::set_visibility(bool value)
{
    visibility = value;
}

void Node2D::set_viewport_model(glm::mat4x4 model)
{
    viewport_model = model;
}

void Node2D::set_priority(uint8_t priority)
{
    class_type = priority;
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

const glm::vec2 Node2D::get_scale() const
{
    glm::mat3x3 model = get_global_model();
    return glm::vec2(glm::length(model[0]), glm::length(model[1]));
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

glm::mat4x4 Node2D::get_global_viewport_model() const
{
    if (parent)
        return parent->get_global_viewport_model() * viewport_model;
    return viewport_model;
}

uint8_t Node2D::get_class_type() const
{
    return class_type;
}

bool Node2D::get_visibility() const
{
    return visibility;
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

Node2D* Node2D::get_widget_from_name(const std::string& name)
{
    if (all_widgets.count(name)) {
        return all_widgets[name];
    }
    return nullptr;
}

void Node2D::clean()
{
    for (auto pair : all_widgets)
    {
        delete pair.second;
    }

    all_widgets.clear();
}

void Node2D::push_input(Node2D* node, sInputData data)
{
    frame_inputs.push_back({ node, data });
}

void Node2D::process_input()
{
    // sort inputs by priority..

    std::sort(frame_inputs.begin(), frame_inputs.end(), [](auto& lhs, auto& rhs) {

        Node2D* lhs_node = lhs.first;
        Node2D* rhs_node = rhs.first;

        // bool equal_priority = lhs_node->get_class_type() == rhs_node->get_class_type();

        if (lhs_node->get_class_type() < rhs_node->get_class_type()) return true;

        return false;
    });

    // call on_input functions..

    for (const auto& i : frame_inputs)
    {
        bool event_processed = i.first->on_input(i.second);
        if (event_processed) {
            break;
        }
    }

    frame_inputs.clear();
}
