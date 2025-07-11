#pragma once

#include "light_3d.h"

class MeshInstance3D;
class Material;
class Surface;

class SpotLight3D : public Light3D {

    float inner_cone_angle = 0.0f;
    float outer_cone_angle = glm::pi<float>() / 4.0f;

    MeshInstance3D* debug_mesh = nullptr;
    Surface* debug_surface = nullptr;

    Material* debug_material = nullptr;

    void create_debug_render_cone();

public:

    SpotLight3D();
    ~SpotLight3D();

    virtual void render() override;

    void clone(Node* new_node, bool copy = true) override;

    void render_gui() override;

    void get_uniform_data(sLightUniformData& data) override;

    float get_inner_cone_angle() const { return inner_cone_angle; };
    float get_outer_cone_angle() const { return outer_cone_angle; };

    void set_range(float value) override;
    void on_set_range() override;

    void set_inner_cone_angle(float value);
    void set_outer_cone_angle(float value);

    void serialize(std::ofstream& binary_scene_file) override;
    void parse(std::ifstream& binary_scene_file) override;

    void create_debug_meshes() override;
};
