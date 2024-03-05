#include "mesh_instance_3d.h"

#include "graphics/renderer.h"

MeshInstance3D::MeshInstance3D() : Node3D()
{
}

MeshInstance3D::~MeshInstance3D()
{
    //for (Surface* surface : surfaces) {
    //    delete surface;
    //}

    //surfaces.clear();
}

void MeshInstance3D::render()
{
    Renderer::instance->add_renderable(this);

    Node3D::render();
}

void MeshInstance3D::update(float delta_time)
{
    Node3D::update(delta_time);
}

void MeshInstance3D::set_surface_material_color(int surface_idx, const glm::vec4& color)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_color(color);
}

void MeshInstance3D::set_surface_material_diffuse(int surface_idx, Texture* diffuse)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_diffuse(diffuse);
    surfaces[surface_idx]->set_material_flag(MATERIAL_DIFFUSE);
}

void MeshInstance3D::set_surface_material_shader(int surface_idx, Shader* shader)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_shader(shader);
}

void MeshInstance3D::set_surface_material_flag(int surface_idx, eMaterialFlags flag)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_flag(flag);
}

void MeshInstance3D::set_surface_material_priority(int surface_idx, uint8_t priority)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    surfaces[surface_idx]->set_material_priority(priority);
}

// Material override

void MeshInstance3D::set_surface_material_override_color(int surface_idx, const glm::vec4& color)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].color = color;
}

void MeshInstance3D::set_surface_material_override_diffuse(int surface_idx, Texture* diffuse)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    Surface* surface = get_surface(surface_idx);
    material_overrides[surface].flags |= MATERIAL_DIFFUSE;
    material_overrides[surface].diffuse_texture = diffuse;
}

void MeshInstance3D::set_surface_material_override_shader(int surface_idx, Shader* shader)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].shader = shader;
}

void MeshInstance3D::set_surface_material_override_flag(int surface_idx, eMaterialFlags flag)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].flags |= flag;
}

void MeshInstance3D::set_surface_material_override_priority(int surface_idx, uint8_t priority)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return;
    }

    material_overrides[get_surface(surface_idx)].priority = priority;
}

void MeshInstance3D::set_surface_material_override(Surface* surface, const Material& material)
{
    material_overrides[surface] = material;
}

Material* MeshInstance3D::get_surface_material_override(Surface* surface)
{
    if (material_overrides.contains(surface))
    {
        return &material_overrides[surface];
    }

    return nullptr;
}

void MeshInstance3D::add_surface(Surface* surface)
{
    surfaces.push_back(surface);
}

Surface* MeshInstance3D::get_surface(int surface_idx) const
{
    assert(surface_idx >= 0 && surface_idx < surfaces.size());

    return surfaces[surface_idx];
}

const std::vector<Surface*>& MeshInstance3D::get_surfaces() const
{
    return surfaces;
}
