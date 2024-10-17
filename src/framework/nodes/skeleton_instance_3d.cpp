#include "skeleton_instance_3d.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "framework/camera/camera.h"
#include "framework/nodes/look_at_ik_3d.h"
#include "framework/animation/skeleton.h"

#include "imgui.h"
#include "framework/utils/ImGuizmo.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "framework/parsers/parse_obj.h"

#include <glm/gtc/type_ptr.hpp>
#include "spdlog/spdlog.h"

SkeletonInstance3D::SkeletonInstance3D()
{
    node_type = "SkeletonInstance3D";

    set_frustum_culling_enabled(false);

    Material* joint_material = new Material();
    joint_material->set_depth_read(false);
    joint_material->set_priority(0);
    joint_material->set_transparency_type(ALPHA_BLEND);
    joint_material->set_color(glm::vec4(1.0f));
    joint_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path));

    joint_render_instance.set_frustum_culling_enabled(false);

    joint_render_instance.add_surface(RendererStorage::get_surface("box"));
    joint_render_instance.set_surface_material_override(joint_render_instance.get_surface(0), joint_material);
}

void SkeletonInstance3D::set_skeleton(Skeleton* s)
{
    skeleton = s;

    init_helper();
}

void SkeletonInstance3D::update(float dt)
{
    if (transform.is_dirty()) {

        update_pose_from_joints(dt);

        transform.set_dirty(false);
    }

    // Update child dones (i.e IK)
    Node3D::update(dt);

    // Update skeleton mesh helper
    update_helper();

    // Update GPU data
    if (animated_uniform_data && invbind_uniform_data)
    {
        const std::vector<glm::mat4x4>& animated_matrices = get_animated_data();
        const std::vector<glm::mat4x4>& inv_bind_matrices = get_invbind_data();
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        webgpu_context->update_buffer(std::get<WGPUBuffer>(animated_uniform_data->data), 0, animated_matrices.data(), sizeof(glm::mat4x4) * animated_matrices.size());
        webgpu_context->update_buffer(std::get<WGPUBuffer>(invbind_uniform_data->data), 0, inv_bind_matrices.data(), sizeof(glm::mat4x4) * inv_bind_matrices.size());
    }

}

void SkeletonInstance3D::update_pose_from_joints(float dt)
{
    Pose& pose = skeleton->get_current_pose();
    const std::vector<uint32_t>& indices = skeleton->get_joint_indices();

    for (size_t i = 0; i < joint_nodes.size(); ++i) {
        joint_nodes[i]->set_transform_dirty(true);
        joint_nodes[i]->update(dt);
        pose.set_local_transform(i, joint_nodes[i]->get_transform());
    }
}

void SkeletonInstance3D::set_joint_nodes(const std::vector<Node3D*>& new_joint_nodes)
{
    joint_nodes = new_joint_nodes;
}

Node* SkeletonInstance3D::get_node(std::vector<std::string>& path_tokens)
{
    if (path_tokens.size() == 0)
        return this;

    assert(path_tokens.size() == 1);

    for (Node3D* joint_node : joint_nodes) {

        if (joint_node->get_name() == path_tokens[0]) {
            return joint_node;
        }
    }

    return nullptr;
}

void SkeletonInstance3D::update_helper()
{
    Surface* s = get_surface(0);

    std::vector<InterleavedData> vertices;

    size_t numJoints = skeleton->get_current_pose().size();
    Pose &pose = skeleton->get_current_pose();

    for (size_t i = 0; i < numJoints; ++i) {
        InterleavedData data;
        data.position = pose.get_global_transform(i).get_position();
        vertices.push_back(data);
        if (pose.get_parent(i) >= 0) {
            data.position = pose.get_global_transform(pose.get_parent(i)).get_position();
        }
        vertices.push_back(data);
    }

    s->update_vertex_buffer(vertices);
}

void SkeletonInstance3D::init_helper()
{
    Surface* s = new Surface();
    s->set_name("Skeleton Helper");
    add_surface(s);

    update_helper();

    Material* skeleton_material = new Material();
    skeleton_material->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });
    skeleton_material->set_depth_read(false);
    skeleton_material->set_priority(0);
    skeleton_material->set_topology_type(eTopologyType::TOPOLOGY_LINE_LIST);
    skeleton_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, skeleton_material));

    set_surface_material_override(s, skeleton_material);
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
            if (changed)
            {
                glm::mat4 parent = c->get_parent() ? c->get_parent()->get_global_model() : glm::mat4(1.f);
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

void SkeletonInstance3D::render() {

    Pose& pose = skeleton->get_current_pose();

    for (size_t i = 0; i < joint_nodes.size(); ++i) {
        Transform joint_transform = pose.get_global_transform(i);
        joint_transform.set_scale(glm::vec3(0.025f));
        Renderer::instance->add_renderable(&joint_render_instance, get_global_model() * joint_transform.get_model());
    }

    MeshInstance3D::render();
}

void SkeletonInstance3D::render_gui()
{
    if (ImGui::Begin(name.c_str())) {

        if (ImGui::TreeNodeEx("Bones", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            ImGui::SetWindowSize(ImVec2(0, 0));
            Pose& pose = skeleton->get_current_pose();
            for (size_t i = 0; i < joint_nodes.size(); i++) {
                int parent = pose.get_parent(i);
                if (parent < 0)
                    continue;
                Node3D* node = (Node3D*)(joint_nodes[i]);
                joint_nodes[parent]->add_child(node);
            }

            for (Node* child : joint_nodes) {
                if(!((Node3D*)(child))->get_parent())
                    recursive_tree_gui(child);
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

