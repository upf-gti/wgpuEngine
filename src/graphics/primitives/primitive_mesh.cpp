#include "primitive_mesh.h"

#include "imgui.h"

PrimitiveMesh::PrimitiveMesh(const glm::vec3& new_color)
    : color(new_color)
{
    mesh_type = "";

    surface = new Surface();

    add_surface(surface);
}

void PrimitiveMesh::set_color(const glm::vec3& new_color)
{
    color = new_color;

    build_mesh();
}

void PrimitiveMesh::render_gui()
{
    ImGui::Text("Color");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::ColorEdit3("##Color", &color.x)) {
        dirty = true;
    }

    Mesh::render_gui();
}
