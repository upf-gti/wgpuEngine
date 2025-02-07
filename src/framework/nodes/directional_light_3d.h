#pragma once

#include "light_3d.h"

class MeshInstance3D;
class Material;

class DirectionalLight3D : public Light3D {

    MeshInstance3D* debug_mesh_h = nullptr;
    MeshInstance3D* debug_mesh_v = nullptr;

    Material* debug_material = nullptr;

public:

    DirectionalLight3D();
    ~DirectionalLight3D();

    virtual void render() override;

    void render_gui() override;

    void set_color(glm::vec3 color) override;

    sLightUniformData get_uniform_data() override;

    void parse(std::ifstream& binary_scene_file) override;

    void create_debug_meshes() override;

};
