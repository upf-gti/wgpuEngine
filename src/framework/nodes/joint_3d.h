#pragma once

#include "framework/nodes/node_3d.h"

class Pose;
class SkeletonInstance3D;
class MeshInstance3D;

class Joint3D : public Node3D {

    uint32_t index = 0;
    Pose* pose = nullptr;

    MeshInstance3D* mesh_instance = nullptr;
    MeshInstance3D* selected_mesh_instance = nullptr;

public:

    static Joint3D* selected_joint;

    Joint3D();
    ~Joint3D();

    Transform get_global_transform() override;
    uint32_t get_index() { return index; }

    void set_index(int32_t new_index) { index = new_index; }
    void set_pose(Pose* ref_pose) { pose = ref_pose; };
    void set_global_transform(const Transform& new_transform) override;

    void render() override;
};

