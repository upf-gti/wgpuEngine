#include "surface_geometry.h"

#include "imgui.h"

SurfaceGeometry::SurfaceGeometry(const glm::vec3& new_color)
    : color(new_color)
{

}

void SurfaceGeometry::set_color(const glm::vec3& new_color)
{
    color = new_color;

    build_mesh();
}

void SurfaceGeometry::render_gui()
{
    ImGui::Text("Color");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::ColorEdit3("##Color", &color.x)) {
        dirty = true;
    }
}
