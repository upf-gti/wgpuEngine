#include "node_2d.h"

#include "framework/input.h"
#include "framework/math/intersections.h"
#include "framework/ui/io.h"
#include "framework/nodes/node_factory.h"
#include "framework/nodes/viewport_3d.h"
#include "framework/ui/ui_utils.h"

#include "graphics/renderer.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/quaternion.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "spdlog/spdlog.h"

#include <algorithm>

unsigned int Node2D::last_uid = 0;

std::map<std::string, Node2D*> Node2D::all_widgets;

REGISTER_NODE_CLASS(Node2D)

Node2D::Node2D(const std::string& n, const glm::vec2& p, const glm::vec2& s, uint32_t parameter_flags) : size(s)
{
    node_type = "Node2D";

    uid = last_uid++;

    name = n;

    all_widgets[name] = this;

    set_position(p);

    if ((parameter_flags & ui::CREATE_3D) && Renderer::instance->get_xr_available()) {
        xr_viewport_3d = new Viewport3D(this);
        xr_viewport_3d->set_active(true);
    }
}

Node2D::~Node2D()
{
    release();

    all_widgets.erase(name);

    if (xr_viewport_3d) {
        delete xr_viewport_3d;
    }
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

    // Disable 2d if rendering in xr
    if (Renderer::instance->get_xr_available()) {
        child->disable_2d();
    }

    child->parent = this;
    children.push_back(child);

    on_children_changed();
}

void Node2D::add_childs(const std::vector<Node2D*>& childs)
{
    for (Node2D* child : childs) {
        add_child(child);
    }
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
    auto parent_2d = dynamic_cast<Node2D*>(parent);
    if (parent_2d) {
        parent_2d->on_children_changed();
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

    if (xr_viewport_3d) {
        xr_viewport_3d->update(delta_time);
    }

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
    scaling = scale;
    model = glm::scale(model, scaling);
}

void Node2D::set_model(const glm::mat3x3& _model)
{
    model = _model;
}

bool Node2D::set_visibility(bool value, bool propagate)
{
    bool last = visibility;

    visibility = value;

    bool changed = (last != value);

    if (propagate && changed) {
        on_children_changed();
    }

    return changed;
}

void Node2D::set_viewport_model(glm::mat4x4 model)
{
    viewport_model = model;
}

void Node2D::set_xr_transform(const Transform& transform)
{
    if (xr_viewport_3d) {
        xr_viewport_3d->set_transform(transform);
    }
    else {
        assert(0 && "Setting transform in non-XR mode! Use ui::CREATE_3D flag when creating Node2D.");
    }
}

void Node2D::set_priority(uint8_t priority)
{
    class_type = priority;
}

void Node2D::set_position(const glm::vec2& translation)
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
    auto parent_2d = dynamic_cast<Node2D*>(parent);
    if (parent_2d) {
		return parent_2d->get_global_model() * model;
    }
	return model;
}

glm::mat4x4 Node2D::get_global_viewport_model() const
{
    auto parent_2d = dynamic_cast<Node2D*>(parent);
    if (parent_2d) {
        return parent_2d->get_global_viewport_model() * viewport_model;
    }
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
    trans = glm::transpose(trans);

    glm::mat4x4 inv = model;
    inv = glm::inverse(model);

    return trans * inv;
}

glm::vec2 Node2D::get_size() const
{
    return size;
}

void Node2D::release()
{
    // Clear event listeners
    Node::unbind(name);
    Node::unbind(name + "@changed");
    Node::unbind(name + "@dbl_click");
    Node::unbind(name + "@pressed");
    Node::unbind(name + "@released");
    Node::unbind(name + "@selected");

    // Node::release();

    if (parent) {
        parent->remove_child(this);
    }
}

void Node2D::clean()
{
    std::vector<Node2D*> to_delete;

    for (auto pair : all_widgets) {

        // it's a child, will be removed later..
        if (pair.second->get_parent()) {
            continue;
        }

        to_delete.push_back(pair.second);
    }

    for (auto node : to_delete) {
        delete node;
    }

    all_widgets.clear();
}
