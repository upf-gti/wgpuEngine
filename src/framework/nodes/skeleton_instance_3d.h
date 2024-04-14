#pragma once

#include "framework/nodes/node_3d.h"
#include "framework/nodes/mesh_instance_3d.h"

#include "framework/animation/skeleton.h"

class SkeletonInstance3D : public MeshInstance3D {

    Skeleton* skeleton = nullptr;

    void init_skeleton_helper();
public:
    int skin = -1;

    SkeletonInstance3D();

    void set_skeleton(Skeleton* skeleton);
    void update(float dt);

    Skeleton* get_skeleton();
};

