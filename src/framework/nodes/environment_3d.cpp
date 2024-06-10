#include "environment_3d.h"
#include "framework/scene/parse_obj.h"
#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"
#include "shaders/mesh_texture_cube.wgsl.gen.h"

Environment3D::Environment3D() : MeshInstance3D()
{
    name = "Environment3D";

    Surface* surface = new Surface;

    surface->create_skybox();

    surfaces.push_back(surface);

    set_surface_material_diffuse(0, Renderer::instance->get_irradiance_texture());
    set_surface_material_cull_type(0, CULL_BACK);
    set_surface_material_depth_write(0, false);
    set_surface_material_priority(0, 20);

    set_surface_material_shader(0, RendererStorage::get_shader_from_source(shaders::mesh_texture_cube::source, shaders::mesh_texture_cube::path, surfaces[0]->get_material()));
}

void Environment3D::update(float delta_time)
{
    Node3D::update(delta_time);

    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_translation(renderer->get_camera_eye());
}

void Environment3D::set_texture(const std::string& texture_path)
{
    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_translation(renderer->get_camera_eye());

    // Change irradiance first
    renderer->set_irradiance_texture(RendererStorage::get_texture(texture_path));

    // Update environment
    set_surface_material_diffuse(0, Renderer::instance->get_irradiance_texture());
}
