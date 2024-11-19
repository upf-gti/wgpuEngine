#pragma once

#include "framework/nodes/mesh_instance_3d.h"

class Skeleton;

class SkeletonInstance3D : public MeshInstance3D {

    Uniform* animated_uniform_data = nullptr;
    Uniform* invbind_uniform_data = nullptr;

    std::vector<Node3D*> joint_nodes;

    int selected_joint_idx = -1;

    void init_helper();
    void update_helper();

    void recursive_tree_gui(Node* node);

    MeshInstance3D joint_render_instance;
    MeshInstance3D selected_joint_render_instance;

public:

    SkeletonInstance3D();

    void update(float dt) override;
    void update_pose_from_joints(float dt);
    void render() override;
    void render_gui() override;

    Skeleton* get_skeleton();
    Node* get_node(std::vector<std::string>& path_tokens) override;
    std::vector<glm::mat4x4> get_animated_data();
    std::vector<glm::mat4x4> get_invbind_data();
    Uniform* get_animated_uniform_data() { return animated_uniform_data; }
    Uniform* get_invbind_uniform_data() { return invbind_uniform_data; }
    Node3D* get_selected_joint() { return joint_nodes[selected_joint_idx]; }
    const Transform& get_selected_joint_transform();

    void set_skeleton(Skeleton* new_skeleton, const std::vector<Node3D*>& new_joint_nodes = {});
    void set_uniform_data(Uniform* animated_u, Uniform* invbind_u);

    bool test_ray_collision(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& distance) override;
};

