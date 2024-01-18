#include "entity_mesh.h"

#include "graphics/renderer.h"

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

void EntityMesh::set_surface_material_color(int surface_idx, const glm::vec4& color)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_color(color);
}

void EntityMesh::set_surface_material_diffuse(int surface_idx, Texture* diffuse)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_diffuse(diffuse);
    surfaces[surface_idx]->set_material_flag(MATERIAL_DIFFUSE);
}

void EntityMesh::set_surface_material_shader(int surface_idx, Shader* shader)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_shader(shader);
}

void EntityMesh::set_surface_material_flag(int surface_idx, eMaterialFlags flag)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_flag(flag);
}

void EntityMesh::set_surface_material_priority(int surface_idx, uint8_t priority)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_priority(priority);
}

// Material override

void EntityMesh::set_surface_material_override_color(int surface_idx, const glm::vec4& color)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].color = color;
}

void EntityMesh::set_surface_material_override_diffuse(int surface_idx, Texture* diffuse)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    Surface* surface = get_surface(surface_idx);
    material_overrides[surface].flags |= MATERIAL_DIFFUSE;
    material_overrides[surface].diffuse_texture = diffuse;
}

void EntityMesh::set_surface_material_override_shader(int surface_idx, Shader* shader)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].shader = shader;
}

void EntityMesh::set_surface_material_override_flag(int surface_idx, eMaterialFlags flag)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].flags |= flag;
}

void EntityMesh::set_surface_material_override_priority(int surface_idx, uint8_t priority)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].priority = priority;
}

void EntityMesh::set_surface_material_override(Surface* surface, const Material& material)
{
    material_overrides[surface] = material;
}

Material* EntityMesh::get_surface_material_override(Surface* surface)
{
    if (material_overrides.contains(surface))
    {
        return &material_overrides[surface];
    }

    return nullptr;
}

void EntityMesh::add_surface(Surface* surface)
{
    surfaces.push_back(surface);
}

Surface* EntityMesh::get_surface(int surface_idx) const
{
    assert(surface_idx >= 0 && surface_idx < surfaces.size());

    return surfaces[surface_idx];
}

const std::vector<Surface*>& EntityMesh::get_surfaces() const
{
    return surfaces;
}
