#include "environment_3d.h"

#include "framework/nodes/node_factory.h"
#include "framework/parsers/parse_obj.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "shaders/mesh_texture_cube.wgsl.gen.h"

REGISTER_NODE_CLASS(Environment3D)

Environment3D::Environment3D() :
        MeshInstance3D()
{
    node_type = "Environment3D";

    name = "Environment3D";
}

void Environment3D::update(float delta_time)
{
    Node3D::update(delta_time);
}

void Environment3D::set_texture(const std::string& texture_path)
{
    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    renderer->set_irradiance_texture(RendererStorage::get_texture(texture_path));
}
