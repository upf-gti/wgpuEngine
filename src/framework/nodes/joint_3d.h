#pragma once

#include "framework/nodes/node_3d.h"

class Pose;
class SkeletonInstance3D;
class MeshInstance3D;

class Joint3D : public Node3D {

    int32_t index = -1;
    SkeletonInstance3D* instance = nullptr;
    Pose* pose = nullptr;

    MeshInstance3D* mesh_instance = nullptr;
    MeshInstance3D* selected_mesh_instance = nullptr;

public:

    Joint3D();
    ~Joint3D();

    Transform get_global_transform() override;

    void set_index(int32_t new_index) { index = new_index; }
    void set_pose(Pose* ref_pose) { pose = ref_pose; };
    void set_instance(SkeletonInstance3D* new_instance) { instance = new_instance; };
    void set_global_transform(const Transform& t);

    void render() override;
    void update_pose();
};

