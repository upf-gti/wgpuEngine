#include "skeleton_instance_3d.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"
#include "graphics/surface.h"

#include "framework/camera/camera.h"
#include "framework/nodes/look_at_ik_3d.h"
#include "framework/nodes/skeleton_helper_3d.h"
#include "framework/nodes/joint_3d.h"
#include "framework/animation/skeleton.h"
#include "framework/math/intersections.h"
#include "framework/parsers/parse_obj.h"
#include "imgui.h"
#include "framework/utils/ImGuizmo.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include <glm/gtc/type_ptr.hpp>
#include "spdlog/spdlog.h"

/*
*   Skeleton Instance
*/

SkeletonInstance3D::SkeletonInstance3D() : Node3D()
{
    node_type = "SkeletonInstance3D";

    collider_shape = COLLIDER_SHAPE_CUSTOM;
}

SkeletonInstance3D::~SkeletonInstance3D()
{
    delete helper;

    if (animated_uniform_data) {
        animated_uniform_data->destroy();
    }

    if (invbind_uniform_data) {
        invbind_uniform_data->destroy();
    }

    skeleton->unref();
}

void SkeletonInstance3D::initialize()
{
    helper->initialize();
}

void SkeletonInstance3D::set_skeleton(Skeleton* new_skeleton, const std::vector<Joint3D*>& new_joint_nodes)
{
    bool is_root = !new_joint_nodes.empty();

    if (is_root) {
        joint_nodes = new_joint_nodes;
        uint32_t idx = 0u;
        for (auto j_node : joint_nodes) {
            j_node->set_index(idx++);
            j_node->set_pose(&new_skeleton->get_current_pose());
            j_node->set_parent(this);
        }
    }

    skeleton = new_skeleton;

    helper = new SkeletonHelper3D(skeleton, this);

    if (!is_root) {
        return;
    }

    // Fill animatable properties from the entire skeleton for root nodes only!

    uint32_t joint_count = new_skeleton->get_joints_count();

    for (uint32_t i = 0u; i < joint_count; ++i) {
        auto joint = joint_nodes[i];
        animatable_properties[joint->get_name() + "$translation"] = { AnimatablePropertyType::FVEC3, &joint->get_transform().get_position_ref() };
        animatable_properties[joint->get_name() + "$rotation"] = { AnimatablePropertyType::QUAT, &joint->get_transform().get_rotation_ref() };
        animatable_properties[joint->get_name() + "$scale"] = { AnimatablePropertyType::FVEC3, &joint->get_transform().get_scale_ref() };
    }
}

void SkeletonInstance3D::update(float delta_time)
{
    if (transform.is_dirty()) {
        update_pose_from_joints();
        transform.set_dirty(false);
    }

    // Update child nodes (i.e IK)
    Node3D::update(delta_time);

    // Update skeleton mesh helper
    helper->update(delta_time);

    // Update GPU data
    if (animated_uniform_data && invbind_uniform_data) {
        const std::vector<glm::mat4x4>& animated_matrices = get_animated_data();
        const std::vector<glm::mat4x4>& inv_bind_matrices = get_invbind_data();
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        webgpu_context->update_buffer(std::get<WGPUBuffer>(animated_uniform_data->data), 0, animated_matrices.data(), sizeof(glm::mat4x4) * animated_matrices.size());
        webgpu_context->update_buffer(std::get<WGPUBuffer>(invbind_uniform_data->data), 0, inv_bind_matrices.data(), sizeof(glm::mat4x4) * inv_bind_matrices.size());
    }
}

void SkeletonInstance3D::render()
{
    for (auto j : joint_nodes) {
        j->render();
    }

    helper->render();

    Node3D::render();
}

void SkeletonInstance3D::update_pose_from_joints()
{
    Pose& pose = skeleton->get_current_pose();

    for (size_t i = 0; i < joint_nodes.size(); ++i) {
        pose.set_local_transform(i, joint_nodes[i]->get_transform());
    }
}

void SkeletonInstance3D::update_joints_from_pose()
{
    Pose& pose = skeleton->get_current_pose();

    for (size_t i = 0; i < joint_nodes.size(); ++i) {
        joint_nodes[i]->set_transform(pose.get_local_transform(i));
    }
}

void SkeletonInstance3D::generate_joints_from_pose()
{
    Pose& pose = skeleton->get_rest_pose();
    const auto& indices = skeleton->get_joint_indices();
    const auto& names = skeleton->get_joint_names();
    uint32_t joint_count = indices.size();

    joint_nodes.resize(joint_count);

    for (size_t i = 0; i < joint_count; i++) {

        Joint3D* joint_3d = new Joint3D();
        joint_3d->set_node_type("Joint3D");
        joint_3d->set_name(names[i]);
        joint_3d->set_index(i);
        joint_3d->set_pose(&skeleton->get_current_pose());
        joint_3d->set_parent(this);
        joint_3d->set_transform(pose.get_local_transform(i));
        joint_nodes[i] = joint_3d;
    }
}

Node* SkeletonInstance3D::get_node(std::vector<std::string>& path_tokens)
{
    if (path_tokens.size() == 0)
        return this;

    // It's "skeleton_instance+joint"
    if (path_tokens.size() == 2) {
        path_tokens.erase(path_tokens.begin());
    }

    for (Node3D* joint_node : joint_nodes) {
        if (joint_node->get_name() == path_tokens[0]) {
            return joint_node;
        }
    }

    if (name == path_tokens[0]) {
        return this;
    }

    return nullptr;
}


Skeleton* SkeletonInstance3D::get_skeleton()
{
    return skeleton;
}

std::vector<glm::mat4x4> SkeletonInstance3D::get_animated_data()
{
    assert(skeleton);
    Pose& current_pose = skeleton->get_current_pose();
    return current_pose.get_global_matrices();
}

std::vector<glm::mat4x4> SkeletonInstance3D::get_invbind_data()
{
    assert(skeleton);
    return skeleton->get_inv_bind_pose();
}

void SkeletonInstance3D::set_uniform_data(Uniform* animated_u, Uniform* invbind_u)
{
    animated_uniform_data = animated_u;
    invbind_uniform_data = invbind_u;
}

bool SkeletonInstance3D::test_ray_collision(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float* distance, Node3D** out)
{
    Pose& pose = skeleton->get_current_pose();

    bool result = false;

    float joint_distance = 1e9f;

    const Transform& global_transform = get_global_transform();

    for (size_t i = 0; i < joint_nodes.size(); ++i) {
        Transform joint_global_transform = Transform::combine(global_transform, pose.get_global_transform(i));
        if (intersection::ray_sphere(ray_origin, ray_direction, joint_global_transform.get_position(), 0.01f, &joint_distance)) {
            if (joint_distance < *distance) {
                *distance = joint_distance;
                result |= true;
                *out = joint_nodes[i];
                Joint3D::selected_joint = static_cast<Joint3D*>(*out);
            }
        }
    }

    return result;
}

void SkeletonInstance3D::recursive_tree_gui(Node* node) {

    ImGuiTreeNodeFlags flags = {};

    bool selected = ((Node3D*)node)->is_selected();
    bool child_selected = ((Node3D*)node)->is_child_selected();

    if (selected || child_selected) {
        flags = { ImGuiTreeNodeFlags_DefaultOpen };
    } 

    if (ImGui::TreeNodeEx(node->get_name().c_str(), flags)) {

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(230, 150, 50)));
        bool is_open = ImGui::TreeNodeEx("Transform", selected ? ImGuiTreeNodeFlags{ ImGuiTreeNodeFlags_DefaultOpen} : ImGuiTreeNodeFlags{});
        ImGui::PopStyleColor();

        bool changed = false;
        Node3D* c = (Node3D*)node;
        Transform transform = c->get_transform();

        if (selected || is_open) {
            Camera* camera = Renderer::instance->get_camera();

            ImGuiIO& io = ImGui::GetIO();
            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
            glm::mat4x4 global_model = get_global_model() * c->get_global_model();
            changed = ImGuizmo::Manipulate(glm::value_ptr(camera->get_view()), glm::value_ptr(camera->get_projection()),
                ImGuizmo::OPERATION::ROTATE, ImGuizmo::MODE::LOCAL, glm::value_ptr(global_model));
            if (changed) {
                glm::mat4 parent = c->get_parent<Node3D*>() ? c->get_parent<Node3D*>()->get_global_model() : glm::mat4(1.f);
                glm::mat4 local_model = inverse(get_global_model() * parent) * global_model;
                c->set_transform(Transform::mat4_to_transform(local_model));
                set_transform_dirty(true);
            }
        }

        if (is_open) {            
            changed = false;
            changed |= ImGui::DragFloat3("Translation", &transform.get_position_ref()[0], 0.1f);
            changed |= ImGui::DragFloat4("Rotation", &transform.get_rotation_ref()[0], 0.1f);
            changed |= ImGui::DragFloat3("Scale", &transform.get_scale_ref()[0], 0.1f);

            if (changed) {
                c->set_transform(transform);
                set_transform_dirty(true);
            }

            ImGui::TreePop();
        }

        for (Node* child : node->get_children()) {
            recursive_tree_gui(child);
        }

        ImGui::TreePop();
    }
}

void SkeletonInstance3D::render_gui()
{
    if (ImGui::Begin(name.c_str())) {

        if (ImGui::TreeNodeEx("Bones", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            ImGui::SetWindowSize(ImVec2(0, 0));
            Pose& pose = skeleton->get_current_pose();
            for (size_t i = 0; i < joint_nodes.size(); i++) {
                int parent = pose.get_parent(i);
                if (parent < 0) {
                    continue;
                }
                Node3D* node = (Node3D*)(joint_nodes[i]);
                joint_nodes[parent]->add_child(node);
            }

            for (Node* child : joint_nodes) {
                if (!child->get_parent()) {
                    recursive_tree_gui(child);
                }
            }     
            ImGui::TreePop();
        }

        ImGui::Separator();

        if(ImGui::Button("Add SkeletonIK"))
        {
            LookAtIK3D* ik_node = new LookAtIK3D(this);
            this->add_child((Node3D*)ik_node);
        }

        ImGui::End();
    }
}

