#pragma once

#include "framework/nodes/node_3d.h"
#include "framework/nodes/mesh_instance_3d.h"

#include "framework/animation/skeleton.h"

class SkeletonInstance3D : public MeshInstance3D {

    void init_helper();
    void update_helper();

    std::vector<Node3D*> joint_nodes;

public:
    int skin = -1;

    SkeletonInstance3D();

    void set_skeleton(Skeleton* skeleton);
    void update(float dt);

    Node* get_node(std::vector<std::string>& path_tokens) override;

    void set_joint_nodes(const std::vector<Node3D*>& new_joint_nodes);

    Skeleton* get_skeleton();
};

