#include "mesh_instance_3d.h"

#include "graphics/renderer.h"

#include "framework/animation/skeleton.h"
#include "framework/nodes/node_factory.h"

REGISTER_NODE_CLASS(MeshInstance3D)

MeshInstance3D::MeshInstance3D() : Node3D()
{
    node_type = "MeshInstance3D";
    mesh_instance = new MeshInstance();
    mesh_instance->set_node_ref(this);
}

MeshInstance3D::~MeshInstance3D()
{

}

void MeshInstance3D::set_aabb(const AABB& new_aabb)
{
    aabb = new_aabb;
}

void MeshInstance3D::update_aabb()
{
    AABB entity_aabb;

    for (const Surface* surface : get_surfaces()) {
        const AABB& surface_aabb = surface->get_aabb();//.transform(get_global_model());
        entity_aabb = merge_aabbs(entity_aabb, surface_aabb);
    }

    set_aabb(entity_aabb);
}

void MeshInstance3D::render()
{
    Renderer::instance->add_renderable(mesh_instance, get_global_transform().get_model());

    Node3D::render();
}

void MeshInstance3D::update(float delta_time)
{  
    Node3D::update(delta_time);
}

void MeshInstance3D::set_surface_material_override(Surface* surface, Material* material)
{
    mesh_instance->set_surface_material_override(surface, material);
}

void MeshInstance3D::set_frustum_culling_enabled(bool enabled)
{
    mesh_instance->set_frustum_culling_enabled(enabled);
}

void MeshInstance3D::set_receive_shadows(bool new_receive_shadows)
{
    mesh_instance->set_receive_shadows(new_receive_shadows);
}

bool MeshInstance3D::get_frustum_culling_enabled()
{
    return mesh_instance->get_frustum_culling_enabled();
}

Material* MeshInstance3D::get_surface_material(int surface_idx)
{
    return mesh_instance->get_surface_material(surface_idx);
}

Material* MeshInstance3D::get_surface_material_override(Surface* surface)
{
    return mesh_instance->get_surface_material_override(surface);
}

const std::vector<Surface*>& MeshInstance3D::get_surfaces() const
{
    return mesh_instance->get_surfaces();
}

Surface* MeshInstance3D::get_surface(int surface_idx) const
{
    return mesh_instance->get_surface(surface_idx);
}

uint32_t MeshInstance3D::get_surface_count() const
{
    return mesh_instance->get_surface_count();
}

void MeshInstance3D::add_surface(Surface* surface)
{
    mesh_instance->add_surface(surface);
}

void MeshInstance3D::render_gui()
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(230, 150, 50)));
    bool is_open = ImGui::TreeNodeEx("Surfaces");
    ImGui::PopStyleColor();

    if (is_open)
    {
        const auto& surfaces = mesh_instance->get_surfaces();
        for (int i = 0; i < surfaces.size(); ++i) {
            std::string surface_name = surfaces[i]->get_name();
            std::string final_surface_name = surface_name.empty() ? ("Surface " + std::to_string(i)).c_str() : surface_name;
            if (ImGui::TreeNodeEx(final_surface_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf)) {
                Material* material = mesh_instance->get_surface_material_override(surfaces[i]);
                if (!material) {
                    material = surfaces[i]->get_material();
                }
                material->render_gui();
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    Node3D::render_gui();
}
