#pragma once

#include "framework/nodes/node_3d.h"

class Uniform;
class Skeleton;
class SkeletonHelper3D;
class Joint3D;

class SkeletonInstance3D : public Node3D {

    Uniform* animated_uniform_data = nullptr;
    Uniform* invbind_uniform_data = nullptr;

    std::vector<Joint3D*> joint_nodes;

    Skeleton* skeleton = nullptr;

    SkeletonHelper3D* helper = nullptr;

    void recursive_tree_gui(Node* node);

public:

    SkeletonInstance3D();
    ~SkeletonInstance3D();

    void update(float delta_time) override;
    void render() override;
    void render_gui() override;

    void update_pose_from_joints();
    void update_joints_from_pose();

    Skeleton* get_skeleton();
    Node* get_node(std::vector<std::string>& path_tokens) override;
    std::vector<glm::mat4x4> get_animated_data();
    std::vector<glm::mat4x4> get_invbind_data();
    Uniform* get_animated_uniform_data() { return animated_uniform_data; }
    Uniform* get_invbind_uniform_data() { return invbind_uniform_data; }

    void set_skeleton(Skeleton* new_skeleton, const std::vector<Joint3D*>& new_joint_nodes = {});
    void set_uniform_data(Uniform* animated_u, Uniform* invbind_u);

    void generate_joints_from_pose();

    bool test_ray_collision(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& distance, Node3D** out = nullptr) override;
};

