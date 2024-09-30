#include "mesh_instance_3d.h"

#include "graphics/renderer.h"
#include "framework/animation/skeleton.h"

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

void MeshInstance3D::render()
{
    Renderer::instance->add_renderable(this, get_global_model());

    Node3D::render();
}

void MeshInstance3D::update(float delta_time)
{  
    Node3D::update(delta_time);
}

void MeshInstance3D::render_gui()
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(230, 150, 50)));
    bool is_open = ImGui::TreeNodeEx("Surfaces");
    ImGui::PopStyleColor();

    if (is_open)
    {
        for (int i = 0; i < surfaces.size(); ++i) {
            std::string surface_name = surfaces[i]->get_name();
            std::string final_surface_name = surface_name.empty() ? ("Surface " + std::to_string(i)).c_str() : surface_name;
            if (ImGui::TreeNodeEx(final_surface_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf)) {
                surfaces[i]->get_material()->render_gui();
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    Node3D::render_gui();
}
