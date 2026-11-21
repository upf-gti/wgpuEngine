#include "mesh_instance_3d.h"

#include "core/managers/render/render_manager.h"

#include "framework/animation/skeleton.h"
#include "scene/main/node_factory.h"

#include "imgui.h"

REGISTER_NODE_CLASS(MeshInstance3D)

void MeshInstance3D::notification(int notification_id)
{
    switch (notification_id) {
        case NOTIFICATION_ENTERS_TREE:
            renderable_data_ref.data.mesh = mesh;
            renderable_data_ref.data.global_matrix = get_global_transform().get_model();
            RENDER_MANAGER->add_renderable(&renderable_data_ref);
            break;
        case NOTIFICATION_EXITS_TREE:
            RENDER_MANAGER->remove_renderable(renderable_data_ref.idx);
            break;
        case NOTIFICATION_TRANSFORM_DIRTY:
            renderable_data_ref.data.global_matrix = get_global_transform().get_model();
            RENDER_MANAGER->set_renderables_dirty();
            break;
    }

    Node::notification(notification_id);
}

MeshInstance3D::MeshInstance3D() : Node3D()
{
    node_type = "MeshInstance3D";
}

void MeshInstance3D::set_aabb(const AABB& new_aabb)
{
    aabb = new_aabb;
}

void MeshInstance3D::update_aabb()
{
    ERR_FAIL_NULL(mesh);

    AABB entity_aabb;

    for (const Surface* surface : mesh->get_surfaces()) {
        const AABB& surface_aabb = surface->get_aabb(); //.transform(get_global_model());
        entity_aabb = merge_aabbs(entity_aabb, surface_aabb);
    }

    set_aabb(entity_aabb);
}

void MeshInstance3D::update(float delta_time)
{
    Node3D::update(delta_time);
}

void MeshInstance3D::set_surface_material_override(Surface* surface, Material* material)
{
    ERR_FAIL_NULL(mesh);
    mesh->set_surface_material_override(surface, material);
}

void MeshInstance3D::set_frustum_culling_enabled(bool enabled)
{
    ERR_FAIL_NULL(mesh);
    mesh->set_frustum_culling_enabled(enabled);
}

void MeshInstance3D::set_receive_shadows(bool new_receive_shadows)
{
    ERR_FAIL_NULL(mesh);
    mesh->set_receive_shadows(new_receive_shadows);
}

void MeshInstance3D::set_mesh(Mesh* new_mesh)
{
    mesh = new_mesh;
    mesh->set_node_ref(this);
}

bool MeshInstance3D::get_frustum_culling_enabled()
{
    ERR_FAIL_NULL_VALUE(mesh);
    return mesh->get_frustum_culling_enabled();
}

Material* MeshInstance3D::get_surface_material(int surface_idx)
{
    ERR_FAIL_NULL_VALUE(mesh);
    return mesh->get_surface_material(surface_idx);
}

Material* MeshInstance3D::get_surface_material_override(Surface* surface)
{
    ERR_FAIL_NULL_VALUE(mesh);
    return mesh->get_surface_material_override(surface);
}

Surface* MeshInstance3D::get_surface(int surface_idx) const
{
    ERR_FAIL_NULL_VALUE(mesh);
    return mesh->get_surface(surface_idx);
}

uint32_t MeshInstance3D::get_surface_count() const
{
    ERR_FAIL_NULL_VALUE(mesh);
    return mesh->get_surface_count();
}

void MeshInstance3D::add_surface(Surface* surface)
{
    if (!mesh) {
        mesh = new Mesh();
        mesh->set_node_ref(this);
    }

    mesh->add_surface(surface);
}

void MeshInstance3D::render_gui()
{
    if (mesh) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(230, 150, 50)));
        bool is_open = ImGui::TreeNodeEx(mesh->get_mesh_type().c_str());
        ImGui::PopStyleColor();

        if (is_open) {
            mesh->render_gui();

            ImGui::TreePop();
        }
    }

    Node3D::render_gui();
}
