#include "environment_3d.h"

#include "framework/parsers/parse_obj.h"
#include "scene/main/node_factory.h"

#include "core/managers/render/render_manager.h"
#include "core/managers/render/render_storage.h"

#include "shaders/mesh_texture_cube.wgsl.gen.h"

REGISTER_NODE_CLASS(Environment3D)

Environment3D::Environment3D() :
        Node3D()
{
    node_type = "Environment3D";

    name = "Environment3D";
}

void Environment3D::update(float delta_time)
{
    Node3D::update(delta_time);
}

void Environment3D::set_sky_texture(Texture* sky_texture)
{
    sky_texture_panorama = sky_texture;
    RenderManager::get_singleton()->environment_data_set_irradiance_texture(sky_texture_panorama);
}
