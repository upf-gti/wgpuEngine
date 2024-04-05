#pragma once

#include "node_3d.h"
#include "graphics/mesh_instance.h"

class Shader;
class Texture;
class Skeleton;

class MeshInstance3D : public MeshInstance, public Node3D {

private:
    Skeleton* skeleton = nullptr;
public:

    bool is_skinned;

    MeshInstance3D();
	virtual ~MeshInstance3D();

	virtual void render() override;
	virtual void update(float delta_time) override;
    void set_skeleton(Skeleton *skeleton);
    Skeleton* get_skeleton();
};
