#include "node_3d.h"

#include "imgui.h"
#include "framework/utils/ImGuizmo.h"
#include "framework/camera/camera.h"

#include "graphics/renderer.h"

#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/quaternion.hpp"

#include "spdlog/spdlog.h"

#include <fstream>

Node3D::Node3D()
{
    node_type = "Node3D";

    properties["translation"] = &transform.get_position_ref();
    properties["rotation"] = &transform.get_rotation_ref();
    properties["scale"] = &transform.get_scale_ref();
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
    Node::update(delta_time);
}

void Node3D::render_gui()
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(230, 150, 50)));
    bool is_open = ImGui::TreeNodeEx("Transform");
    ImGui::PopStyleColor();

    if (is_open || selected)
    {
        glm::mat4x4 test_model = get_model();
        Camera* camera = Renderer::instance->get_camera();

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        bool changed = ImGuizmo::Manipulate(glm::value_ptr(camera->get_view()), glm::value_ptr(camera->get_projection()),
            ImGuizmo::OPERATION::ROTATE, ImGuizmo::MODE::WORLD, glm::value_ptr(test_model));

        if (changed)
        {
            transform = Transform::mat4_to_transform(test_model);
        }

        changed = false;

        changed |= ImGui::DragFloat3("Translation", &transform.get_position_ref()[0], 0.1f);
        changed |= ImGui::DragFloat4("Rotation", &transform.get_rotation_ref()[0], 0.1f);
        changed |= ImGui::DragFloat3("Scale", &transform.get_scale_ref()[0], 0.1f);

        if (changed)
        {
            transform.set_dirty(true);
        }


        ImGui::TreePop();
    }
}

void Node3D::translate(const glm::vec3& translation)
{
    transform.translate(translation);
}

void Node3D::rotate(float angle, const glm::vec3& axis)
{
    transform.rotate(glm::angleAxis(angle, axis));
}

void Node3D::rotate(const glm::quat& q)
{
    transform.rotate(q);
}

void Node3D::scale(glm::vec3 scale)
{
    transform.scale(scale);
}

void Node3D::serialize(std::ofstream& binary_scene_file)
{
    Node::serialize(binary_scene_file);

    binary_scene_file.write(reinterpret_cast<char*>(&(transform.get_position_ref())), sizeof(glm::vec3));
    binary_scene_file.write(reinterpret_cast<char*>(&transform.get_rotation_ref()), sizeof(glm::quat));
    binary_scene_file.write(reinterpret_cast<char*>(&transform.get_scale_ref()), sizeof(glm::vec3));
}

void Node3D::parse(std::ifstream& binary_scene_file)
{
    Node::parse(binary_scene_file);

    binary_scene_file.read(reinterpret_cast<char*>(&(transform.get_position_ref())), sizeof(glm::vec3));
    binary_scene_file.read(reinterpret_cast<char*>(&transform.get_rotation_ref()), sizeof(glm::quat));
    binary_scene_file.read(reinterpret_cast<char*>(&transform.get_scale_ref()), sizeof(glm::vec3));

    transform.set_dirty(true);
}

void Node3D::set_transform_dirty(bool value)
{
    transform.set_dirty(value);

    for (Node* child : children) {
        Node3D* node_3d = dynamic_cast<Node3D*>(child);
        if (node_3d) {
            node_3d->set_transform_dirty(value);
        }
    }
}

void Node3D::set_position(const glm::vec3& translation)
{
    transform.set_position(translation);
}

void Node3D::set_scale(const glm::vec3& scale)
{
    transform.set_scale(scale);
}

void Node3D::set_transform(const Transform& new_transform)
{
    transform = new_transform;

    set_transform_dirty(true);
}

void Node3D::set_parent(Node3D* parent)
{
    this->parent = parent;
}

const glm::vec3 Node3D::get_local_translation() const
{
    return transform.get_position();
}

const glm::vec3 Node3D::get_translation()
{
    return get_global_model()[3];
}

glm::mat4x4 Node3D::get_global_model()
{
    if (parent)
        return parent->get_global_model() * transform.get_model();
    return transform.get_model();
}

glm::mat4x4 Node3D::get_model()
{
    return transform.get_model();
}

glm::quat Node3D::get_rotation() const
{
    return transform.get_rotation();
}

Node3D* Node3D::get_parent() const
{
    return parent;
}

const Transform& Node3D::get_transform() const
{
    return transform;
}

void Node3D::select()
{
    selected = true;
}

void Node3D::unselect()
{
    selected = false;
}

bool Node3D::is_selected()
{
    return selected;
}

bool Node3D::is_child_selected()
{
    if (selected)
        return selected;

    for (auto& child : children) {
        if (((Node3D*)child)->is_child_selected())
            return true;
    }
    return false;
}
