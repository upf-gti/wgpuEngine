#include "environment_3d.h"

#include "framework/parsers/parse_obj.h"
#include "framework/nodes/node_factory.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "shaders/mesh_texture_cube.wgsl.gen.h"

REGISTER_NODE_CLASS(Environment3D)

Environment3D::Environment3D() : MeshInstance3D()
{
    node_type = "Environment3D";

    name = "Environment3D";

    mesh_instance->set_frustum_culling_enabled(false);

    Material* material = new Material();

    material->set_diffuse_texture(Renderer::instance->get_irradiance_texture());
    material->set_cull_type(CULL_BACK);
    material->set_type(MATERIAL_UNLIT);
    material->set_depth_write(false);
    material->set_priority(20);
    material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_texture_cube::source, shaders::mesh_texture_cube::path, shaders::mesh_texture_cube::libraries, material));

    Surface* surface = new Surface();

    surface->create_skybox();
    surface->set_material(material);

    add_surface(surface);
}

void Environment3D::update(float delta_time)
{
    Node3D::update(delta_time);

    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_position(renderer->get_camera_eye());
}

Texture* Environment3D::get_texture() const
{
    Material* material = mesh_instance->get_surface_material(0);
    return material->get_diffuse_texture();
}

void Environment3D::set_texture(const std::string& texture_path)
{
    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_position(renderer->get_camera_eye());

    // Change irradiance first
    renderer->set_irradiance_texture(RendererStorage::get_texture(texture_path));

    get_surface_material(0)->set_diffuse_texture(Renderer::instance->get_irradiance_texture());
}
