#pragma once

#include "node_3d.h"
#include "graphics/mesh_instance.h"

class Shader;
class Texture;
class Skeleton;

class MeshInstance3D : public MeshInstance, public Node3D {

public:

    bool is_skinned = false;

    Uniform* animated_uniform_data = nullptr;
    Uniform* invbind_uniform_data = nullptr;

    MeshInstance3D();
	virtual ~MeshInstance3D();

	virtual void render() override;
	virtual void update(float delta_time) override;

    std::vector<glm::mat4x4> get_animated_data();
    std::vector<glm::mat4x4> get_invbind_data();

    Uniform* get_animated_uniform_data() { return animated_uniform_data; }
    Uniform* get_invbind_uniform_data() { return invbind_uniform_data; }

    void set_uniform_data(Uniform* animated_uniform, Uniform* invbind_uniform);
};
