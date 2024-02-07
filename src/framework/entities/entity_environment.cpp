#include "entity_environment.h"

#include "framework/scene/parse_obj.h"
#include "graphics/renderer.h"

EntityEnvironment::EntityEnvironment() : EntityMesh()
{
    parse_obj("data/meshes/cube.obj", this);

    set_surface_material_shader(0, RendererStorage::get_shader("data/shaders/mesh_texture_cube.wgsl"));

    set_surface_material_diffuse(0, Renderer::instance->get_irradiance_texture());

    scale(glm::vec3(100.f));

    set_surface_material_priority(0, 2);
}

void EntityEnvironment::render()
{
    if (!active) return;

    Renderer::instance->add_renderable(this);

    Entity::render();
}

void EntityEnvironment::update(float delta_time)
{
    if (!active) return;

    Entity::update(delta_time);

    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_translation(renderer->get_camera_eye());
}

void EntityEnvironment::set_texture(const std::string& texture_path)
{
    Renderer* renderer = static_cast<Renderer*>(Renderer::instance);
    set_translation(renderer->get_camera_eye());

    // Change irradiance first
    renderer->set_irradiance_texture(RendererStorage::get_texture(texture_path));

    // Update environment
    set_surface_material_diffuse(0, Renderer::instance->get_irradiance_texture());
}
