#include "mesh.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"

Mesh::Mesh()
{
    mesh_type = "Mesh";
}

Mesh::~Mesh()
{
    for (auto& material_override : material_overrides) {
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

Material* Mesh::get_surface_material(int surface_idx)
{
    assert(surface_idx < surfaces.size());

    if (surface_idx >= surfaces.size()) {
        return nullptr;
    }

    return surfaces[surface_idx]->get_material();
}

void Mesh::set_surface_material_override(Surface* surface, Material* material)
{
    material->ref();
    material_overrides[surface] = material;
}

void Mesh::set_frustum_culling_enabled(bool enabled)
{
    frustum_culling_enabled = enabled;
}

bool Mesh::get_frustum_culling_enabled() const
{
    return frustum_culling_enabled;
}

Material* Mesh::get_surface_material_override(Surface* surface)
{
    if (material_overrides.contains(surface)) {
        return material_overrides[surface];
    }

    return nullptr;
}

void Mesh::add_surface(Surface* surface)
{
    surface->ref();
    surfaces.push_back(surface);
}

Surface* Mesh::get_surface(int surface_idx) const
{
    assert(surface_idx >= 0 && surface_idx < surfaces.size());

    return surfaces[surface_idx];
}

uint32_t Mesh::get_surface_count() const
{
    return (uint32_t)surfaces.size();
}

const std::vector<Surface*>& Mesh::get_surfaces() const
{
    return surfaces;
}

std::vector<Surface*>& Mesh::get_surfaces()
{
    return surfaces;
}

void Mesh::set_skeleton(Skeleton* s)
{
    skeleton = s;
}

Skeleton* Mesh::get_skeleton() const
{
    return skeleton;
}

void Mesh::render_gui()
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(230, 150, 50)));
    bool is_open = ImGui::TreeNodeEx("Surfaces");
    ImGui::PopStyleColor();

    if (is_open)
    {
        for (int i = 0; i < surfaces.size(); ++i) {
            Surface* surface = surfaces[i];
            std::string surface_name = surface->get_name();
            std::string final_surface_name = surface_name.empty() ? ("Surface " + std::to_string(i)).c_str() : surface_name;
            if (ImGui::TreeNodeEx(final_surface_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

                surface->render_gui();

                ImGui::Checkbox("Receive Shadows", &receive_shadows);
                ImGui::Checkbox("Frustum Culling", &frustum_culling_enabled);

                Material* material = get_surface_material_override(surface);
                if (!material) {
                    material = surface->get_material();
                }
                material->render_gui();
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }
}
