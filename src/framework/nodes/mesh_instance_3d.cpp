#include "mesh_instance_3d.h"

#include "graphics/renderer.h"

#include "framework/animation/skeleton.h"
#include "framework/nodes/node_factory.h"

REGISTER_NODE_CLASS(MeshInstance3D)

MeshInstance3D::MeshInstance3D() : Node3D()
{
    node_type = "MeshInstance3D";
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
    if (mesh) {
        Renderer::instance->add_renderable(mesh, get_global_transform().get_model());
    }

    Node3D::render();
}

void MeshInstance3D::update(float delta_time)
{
    Node3D::update(delta_time);
}

void MeshInstance3D::set_surface_material_override(Surface* surface, Material* material)
{
    assert(mesh);
    mesh->set_surface_material_override(surface, material);
}

void MeshInstance3D::set_frustum_culling_enabled(bool enabled)
{
    assert(mesh);
    mesh->set_frustum_culling_enabled(enabled);
}

void MeshInstance3D::set_receive_shadows(bool new_receive_shadows)
{
    assert(mesh);
    mesh->set_receive_shadows(new_receive_shadows);
}

void MeshInstance3D::set_mesh(Mesh* new_mesh)
{
    mesh = new_mesh;
    mesh->set_node_ref(this);
}

bool MeshInstance3D::get_frustum_culling_enabled()
{
    assert(mesh);
    return mesh->get_frustum_culling_enabled();
}

Material* MeshInstance3D::get_surface_material(int surface_idx)
{
    assert(mesh);
    return mesh->get_surface_material(surface_idx);
}

Material* MeshInstance3D::get_surface_material_override(Surface* surface)
{
    assert(mesh);
    return mesh->get_surface_material_override(surface);
}

const std::vector<Surface*>& MeshInstance3D::get_surfaces() const
{
    assert(mesh);
    return mesh->get_surfaces();
}

Surface* MeshInstance3D::get_surface(int surface_idx) const
{
    assert(mesh);
    return mesh->get_surface(surface_idx);
}

uint32_t MeshInstance3D::get_surface_count() const
{
    assert(mesh);
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

        if (is_open)
        {
            mesh->render_gui();

            ImGui::TreePop();
        }
    }

    Node3D::render_gui();
}
