#pragma once

#include "framework/nodes/node_3d.h"
#include "framework/nodes/mesh_instance_3d.h"

#include "framework/animation/skeleton.h"

class SkeletonInstance3D : public MeshInstance3D {

    void init_helper();
    void update_helper();

public:
    int skin = -1;
    std::vector<Node3D*> joint_nodes;

    SkeletonInstance3D();

    void set_skeleton(Skeleton* skeleton);
    void update(float dt);

    void update_pose_from_joints();

    Node* get_node(std::vector<std::string>& path_tokens) override;

    void set_joint_nodes(const std::vector<Node3D*>& new_joint_nodes);

    Skeleton* get_skeleton();
};

