#include "entity_mesh.h"

#include "graphics/renderer.h"
#include "graphics/mesh.h"

EntityMesh::EntityMesh() : Entity()
{
}

void EntityMesh::render()
{
    if (!active) return;

    Renderer::instance->add_renderable(this);

    Entity::render();
}

void EntityMesh::update(float delta_time)
{
    if (!active) return;

    Entity::update(delta_time);
}

void EntityMesh::set_material_color(int surface_idx, const glm::vec4& color)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx].material.color = color;
}

void EntityMesh::set_material_diffuse(int surface_idx, Texture* diffuse)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx].material.diffuse_texture = diffuse;
    surfaces[surface_idx].material.flags |= MATERIAL_DIFFUSE;
}

void EntityMesh::set_material_shader(int surface_idx, Shader* shader)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx].material.shader = shader;
}

void EntityMesh::set_material_flag(int surface_idx, eMaterialFlags flag)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx].material.flags |= flag;
}

void EntityMesh::set_material_priority(int surface_idx, uint8_t priority)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx].material.priority = priority;
}

void EntityMesh::add_surface(const Surface& surface)
{
    surfaces.push_back(surface);
}

Surface& EntityMesh::get_surface(int surface_idx)
{
    assert(surface_idx < surfaces.size());

    return surfaces[surface_idx];
}
