#pragma once

#include "mesh_instance_3d.h"

class Environment3D : public Node3D {
public:
    Environment3D();
    virtual ~Environment3D() {}

    void update(float delta_time) override;

    void set_sky_texture(Texture* sky_texture);

private:
    Texture* sky_texture_panorama = nullptr;
};
