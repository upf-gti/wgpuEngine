#include "skeleton_instance_3d.h"

#include "graphics/renderer.h"
#include "imgui.h"
#include "spdlog/spdlog.h"

SkeletonInstance3D::SkeletonInstance3D()
{

}

void SkeletonInstance3D::set_skeleton(Skeleton* s)
{
    skeleton = s;

    init_helper();
}

void SkeletonInstance3D::update(float dt)
{
    update_helper();

    if (model_dirty) {

        update_pose_from_joints();

        model_dirty = false;
    }

    Node3D::update(dt);
}

void SkeletonInstance3D::update_pose_from_joints()
{
    Pose& pose = skeleton->get_current_pose();
    const std::vector<uint32_t>& indices = skeleton->get_joint_indices();

    for (size_t i = 0; i < joint_nodes.size(); ++i) {
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

    std::vector<InterleavedData>& vertices = s->get_vertices();
    vertices.clear();
    vertices.resize(0);

    unsigned int numJoints = skeleton->get_current_pose().size();
    Pose pose = skeleton->get_current_pose();

    glm::mat4x4 global_model = get_global_model();

    for (unsigned int i = 0; i < numJoints; ++i) {
        InterleavedData data;

        data.position = pose.get_global_transform(i).position;
        vertices.push_back(data);
        if (pose.get_parent(i) >= 0) {
            data.position = pose.get_global_transform(pose.get_parent(i)).position;
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

    Material skeleton_material;
    skeleton_material.color = { 1.0f, 0.0f, 0.0f, 1.0f };
    skeleton_material.depth_read = false;
    skeleton_material.transparency_type = eTransparencyType::ALPHA_BLEND;
    skeleton_material.topology_type = eTopologyType::TOPOLOGY_LINE_LIST;
    skeleton_material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl", skeleton_material);

    set_surface_material_override(s, skeleton_material);
}

Skeleton* SkeletonInstance3D::get_skeleton()
{
    return skeleton;
}


void recursive_tree_gui(Node* node) {
    if (ImGui::TreeNode(node->get_name().c_str())) {
        Node3D* c = (Node3D*)node;
        Transform t = c->get_transform();
        ImGui::DragFloat3("Translation", &t.position[0], 0.f);
        ImGui::DragFloat4("Rotation", &t.rotation[0], 0.f);
        ImGui::DragFloat3("Scale", &t.scale[0], 0.f);
        for (Node* child : node->get_children()) {

            recursive_tree_gui(child);
        }
        ImGui::TreePop();
    }
}

void SkeletonInstance3D::render_gui() {

    ImGui::Begin(name.c_str());
    if (ImGui::TreeNode(name.c_str())) {
        Pose& pose = skeleton->get_current_pose();
        for (size_t i = 0; i < joint_nodes.size(); i++) {
            int parent = pose.get_parent(i);
            if (parent < 0)
                continue;
            Node3D* node = (Node3D*)(joint_nodes[i]);
            joint_nodes[parent]->add_child(node);
        }
        /*for (const Node* child : joint_nodes) {
            if (ImGui::TreeNode(child->get_name().c_str())) {
                Node3D* c = (Node3D*)child;
                Transform t = c->get_transform();
                ImGui::DragFloat3("Translation", &t.position[0], 0.f);
                ImGui::DragFloat4("Rotation", &t.rotation[0], 0.f);
                ImGui::DragFloat3("Scale", &t.scale[0], 0.f);
                ImGui::TreePop();
            }
        }*/
        for (Node* child : joint_nodes) {
            if(!((Node3D*)(child))->get_parent())
                recursive_tree_gui(child);
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

