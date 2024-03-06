#include "environment_3d.h"
#include "framework/scene/parse_obj.h"
#include "graphics/renderer.h"

Environment3D::Environment3D() : MeshInstance3D()
{
    parse_obj("data/meshes/cube.obj", this);

    set_surface_material_diffuse(0, Renderer::instance->get_irradiance_texture());

    set_surface_material_priority(0, 2);

    set_surface_material_shader(0, RendererStorage::get_shader("data/shaders/mesh_texture_cube.wgsl"));

    scale(glm::vec3(100.f));
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
