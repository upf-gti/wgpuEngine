#include "mesh_instance.h"

MeshInstance::MeshInstance()
{

}

MeshInstance::~MeshInstance()
{
    for (Surface* surface : surfaces) {
       surface->unref();
    }

    surfaces.clear();
}

Material* MeshInstance::get_surface_material(int surface_idx)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return nullptr;
    }

    return surfaces[surface_idx]->get_material();
}

void MeshInstance::set_surface_material_override(Surface* surface, Material* material)
{
    material->ref();
    material_overrides[surface] = material;
}

Material* MeshInstance::get_surface_material_override(Surface* surface)
{
    if (material_overrides.contains(surface))
    {
        return material_overrides[surface];
    }

    return nullptr;
}

void MeshInstance::add_surface(Surface* surface)
{
    surface->ref();
    surface->set_index(get_surface_count());
    surfaces.push_back(surface);
}

Surface* MeshInstance::get_surface(int surface_idx) const
{
    assert(surface_idx >= 0 && surface_idx < surfaces.size());

    return surfaces[surface_idx];
}

uint32_t MeshInstance::get_surface_count() const
{
    return (uint32_t)surfaces.size();
}

const std::vector<Surface*>& MeshInstance::get_surfaces() const
{
    return surfaces;
}

void MeshInstance::set_skeleton(Skeleton* s)
{
    skeleton = s;
}

Skeleton* MeshInstance::get_skeleton() {
    return skeleton;
}
