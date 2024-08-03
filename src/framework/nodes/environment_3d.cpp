#include "environment_3d.h"
#include "framework/scene/parse_obj.h"
#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"
#include "shaders/mesh_texture_cube.wgsl.gen.h"

Environment3D::Environment3D() : MeshInstance3D()
{
    node_type = "Environment3D";

    name = "Environment3D";

    Material* material = new Material();

    material->diffuse_texture = Renderer::instance->get_irradiance_texture();
    material->cull_type = CULL_BACK;
    material->type = MATERIAL_UNLIT;
    material->depth_write = false;
    material->priority = 20;
    material->shader = RendererStorage::get_shader_from_source(shaders::mesh_texture_cube::source, shaders::mesh_texture_cube::path, material);

    Surface* surface = new Surface;

    surface->create_skybox();
    surface->set_material(material);

    surfaces.push_back(surface);
}

void Environment3D::update(float delta_time)
{
    Node3D::update(delta_time);

    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_position(renderer->get_camera_eye());
}

void Environment3D::set_texture(const std::string& texture_path)
{
    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_position(renderer->get_camera_eye());

    // Change irradiance first
    renderer->set_irradiance_texture(RendererStorage::get_texture(texture_path));

    get_surface_material(0)->diffuse_texture = Renderer::instance->get_irradiance_texture();
}
