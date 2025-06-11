#include "box_geometry.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

BoxGeometry::BoxGeometry(float width, float height, float depth, const glm::vec3& color)
    : SurfaceGeometry(color), width(width), height(height), depth(depth)
{
    build_mesh();
}

void BoxGeometry::build_mesh()
{
    create_box(width, height, depth, color);
}

void BoxGeometry::set_width(float new_width)
{
    width = new_width;
    build_mesh();
}

void BoxGeometry::set_height(float new_height)
{
    height = new_height;
    build_mesh();
}

void BoxGeometry::set_depth(float new_depth)
{
    depth = new_depth;
    build_mesh();
}

void BoxGeometry::set_size(const glm::vec3& size)
{
    width = size.x;
    height = size.y;
    depth = size.z;
    build_mesh();
}

void BoxGeometry::render_gui()
{
    Surface::render_gui();

    std::string surface_name = name.empty() ? "" : (" (" + name + ")");
    if (ImGui::TreeNodeEx(("Geometry" + surface_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::Text("Width");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Width", &width, 0.01f, 0.01f, 2.0f)) {
            dirty = true;
        }

        ImGui::Text("Height");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Height", &height, 0.01f, 0.01f, 2.0f)) {
            dirty = true;
        }

        ImGui::Text("Depth");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Depth", &depth, 0.01f, 0.01f, 2.0f)) {
            dirty = true;
        }

        ImGui::TreePop();
    }

    if (dirty) {
        build_mesh();
        dirty = false;
    }
}
