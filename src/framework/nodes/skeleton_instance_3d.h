#pragma once

#include "framework/nodes/node_3d.h"
#include "framework/nodes/mesh_instance_3d.h"

#include "framework/animation/skeleton.h"

class SkeletonInstance3D : public Node3D {

    Skeleton* skeleton = nullptr;
    MeshInstance3D* skeleton_helper = nullptr;

    void init_skeleton_helper();
public:
    int skin = -1;

    SkeletonInstance3D()  {};
    virtual ~SkeletonInstance3D() {};

    virtual void render();
    virtual void update(float delta_time);

    void set_skeleton(Skeleton* skeleton);

    Skeleton* get_skeleton();
};

