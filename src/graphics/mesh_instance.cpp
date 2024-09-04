#include "mesh_instance.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"

MeshInstance::MeshInstance()
{

}

MeshInstance::~MeshInstance()
{
    for (auto &material_override : material_overrides) {
        if (material_override.second->unref()) {
            RendererStorage::delete_material_bind_group(Renderer::instance->get_webgpu_context(), material_override.second);
        }
    }

    for (Surface* surface : surfaces) {
       surface->unref();
    }

    surfaces.clear();
    material_overrides.clear();
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

void MeshInstance::set_frustum_culling_enabled(bool enabled)
{
    frustum_culling_enabled = enabled;
}

bool MeshInstance::get_frustum_culling_enabled()
{
    return frustum_culling_enabled;
}

Material* MeshInstance::get_surface_material_override(Surface* surface)
{
    if (material_overrides.contains(surface)) {
        return material_overrides[surface];
    }

    return nullptr;
}

void MeshInstance::add_surface(Surface* surface)
{
    surface->ref();
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
