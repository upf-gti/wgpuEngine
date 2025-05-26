#include "node_3d.h"

#include "imgui.h"

#include "framework/utils/ImGuizmo.h"
#include "framework/camera/camera.h"
#include "framework/math/intersections.h"
#include "framework/nodes/node_factory.h"

#include "graphics/renderer.h"

#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"

#include <fstream>

REGISTER_NODE_CLASS(Node3D)

Node3D::Node3D()
{
    if (node_type.empty()) {
        node_type = "Node3D";
    }

    animatable_properties["translation"] = { AnimatablePropertyType::FVEC3, &transform.get_position_ref() };
    animatable_properties["rotation"] = { AnimatablePropertyType::QUAT, &transform.get_rotation_ref() };
    animatable_properties["scale"] = { AnimatablePropertyType::FVEC3, &transform.get_scale_ref() };
}

Node3D::~Node3D()
{
    for (Node* child : children) {
        delete child;
    }

    children.clear();
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

        glm::vec3 position = transform.get_position();
        glm::quat rotation = transform.get_rotation();
        glm::vec3 scale = transform.get_scale();

        glm::mat4 m = glm::mat4_cast(rotation);

        float y, p, r;
        glm::extractEulerAngleYXZ(m, y, p, r);

        // Package them as a vector for interface similarity with
        // `glm::eulerAngles` (but the order is different!).
        glm::vec3 euler_rotation = glm::vec3{ p, y, r };

        if (ImGui::DragFloat3("Translation", &position[0], 0.1f)) {
            set_position(position);
        }

        if (ImGui::DragFloat3("Rotation", &euler_rotation[0], 0.01f)) {
            glm::vec3 delta_rot_euler = euler_rotation - glm::vec3{ p, y, r };
            glm::quat delta_rot_quat = glm::quat(delta_rot_euler);

            rotation = delta_rot_quat * rotation;
            set_rotation(rotation);
        }

        if (ImGui::DragFloat3("Scale", &scale[0], 0.1f)) {
            set_scale(scale);
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

void Node3D::rotate_world(const glm::quat& q)
{
    transform.rotate_world(q);
}

void Node3D::scale(const glm::vec3& scale)
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

void Node3D::set_rotation(const glm::quat& rotation)
{
    transform.set_rotation(rotation);
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

void Node3D::set_global_transform(const Transform& new_transform)
{
    Transform local_transform = new_transform;

    if (parent) {
        local_transform = Transform::combine(Transform::inverse(get_parent<Node3D*>()->get_global_transform()), local_transform);
    }

    set_transform(local_transform);
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
    return get_global_transform().get_position();
}

glm::mat4x4 Node3D::get_global_model()
{
    if (parent) {
        return static_cast<Node3D*>(parent)->get_global_model() * transform.get_model();
    }
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

Transform& Node3D::get_transform()
{
    return transform;
}

Transform Node3D::get_global_transform()
{
    if (parent) {
        return Transform::combine(static_cast<Node3D*>(parent)->get_global_transform(), transform);
    }
    return transform;
}

bool Node3D::test_ray_collision(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float* distance, Node3D** out)
{
    const glm::vec3& center = aabb.center + transform.get_position();
    const glm::vec3& half_size = aabb.half_size * transform.get_scale();

    if (collider_shape == COLLIDER_SHAPE_AABB) {
        return intersection::ray_AABB(ray_origin, ray_direction, center, half_size, distance);
    } else if (collider_shape == COLLIDER_SHAPE_SPHERE) {
        return intersection::ray_sphere(ray_origin, ray_direction, center, 0.1f, distance);
    }

    return false;
}

void Node3D::clone(Node* new_node, bool copy)
{
    Node::clone(new_node, copy);

    Node3D* new_node_3d = static_cast<Node3D*>(new_node);

    new_node_3d->set_transform(get_transform());
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
