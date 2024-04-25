#include "node_3d.h"

#include "imgui.h"
#include "framework/utils/ImGuizmo.h"
#include "framework/camera/camera.h"

#include "graphics/renderer.h"

#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/quaternion.hpp"

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
        child->parent->remove_child(child);
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
    if (model_dirty) {

        set_model(transformToMat4(transform));

        model_dirty = false;
    }

    Node::update(delta_time);
}

void Node3D::render_gui()
{
    if (ImGui::TreeNodeEx("Transform"))
    {
        glm::mat4x4 test_model = get_model();
        Camera* camera = Renderer::instance->get_camera();

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        bool changed = ImGuizmo::Manipulate(glm::value_ptr(camera->get_view()), glm::value_ptr(camera->get_projection()),
            ImGuizmo::OPERATION::ROTATE, ImGuizmo::MODE::WORLD, glm::value_ptr(test_model));

        if (changed)
        {
            set_model(test_model);
            set_transform(mat4ToTransform(test_model));
        }

        changed = false;

        changed |= ImGui::DragFloat3("Translation", &transform.position[0], 0.1f);
        changed |= ImGui::DragFloat4("Rotation", &transform.rotation[0], 0.1f);
        changed |= ImGui::DragFloat3("Scale", &transform.scale[0], 0.1f);

        if (changed)
        {
            set_model(transformToMat4(transform));
        }

        ImGui::TreePop();
    }
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

void Node3D::set_model_dirty(bool value)
{
    model_dirty = value;

    for (Node* child : children) {
        Node3D* node_3d = dynamic_cast<Node3D*>(child);
        if (node_3d) {
            node_3d->set_model_dirty(value);
        }
    }
}

void Node3D::set_translation(const glm::vec3& translation)
{
    model = glm::translate(glm::mat4x4(1.f), translation);
}

void Node3D::set_parent(Node3D* parent) {
    this->parent = parent;
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


const Transform& Node3D::get_transform() const
{
    return transform;
}
