#pragma once

#include "framework/nodes/mesh_instance_3d.h"

class Skeleton;

class SkeletonHelper3D : public MeshInstance3D {

    Skeleton* skeleton = nullptr;

    void inner_update();

public:

    SkeletonHelper3D(Skeleton* new_skeleton, Node3D* parent = nullptr);

    virtual void initialize() override;

    void update(float delta_time) override;
};

