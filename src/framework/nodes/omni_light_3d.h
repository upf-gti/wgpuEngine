#pragma once

#include "light_3d.h"

class MeshInstance3D;
class Material;

class OmniLight3D : public Light3D {

    MeshInstance3D* debug_mesh_h = nullptr;
    MeshInstance3D* debug_mesh_v = nullptr;

    Material* debug_material = nullptr;

public:

    OmniLight3D();
    ~OmniLight3D();

    virtual void render() override;

    void render_gui() override;

    void set_color(glm::vec3 color) override;
    void set_range(float value) override;

    sLightUniformData get_uniform_data() override;
};
