#include "cylinder_geometry.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

CylinderGeometry::CylinderGeometry(float radius, float height, uint32_t rings, uint32_t ring_segments, bool capped, const glm::vec3& color)
    : SurfaceGeometry(color), radius(radius), height(height), rings(rings), ring_segments(ring_segments), capped(capped)
{
    build_mesh();
}

void CylinderGeometry::build_mesh()
{
    create_cylinder(radius, height, rings, ring_segments, capped, color);
}

void CylinderGeometry::set_radius(float new_radius)
{
    radius = new_radius;
    build_mesh();
}

void CylinderGeometry::set_height(float new_height)
{
    height = new_height;
    build_mesh();
}

void CylinderGeometry::set_rings(uint32_t new_rings)
{
    rings = new_rings;
    build_mesh();
}

void CylinderGeometry::set_ring_segments(uint32_t new_ring_segments)
{
    ring_segments = new_ring_segments;
    build_mesh();
}

void CylinderGeometry::set_capped(bool new_capped)
{
    capped = new_capped;
    build_mesh();
}

void CylinderGeometry::render_gui()
{
    std::string surface_name = name.empty() ? "" : (" (" + name + ")");
    if (ImGui::TreeNodeEx(("Geometry" + surface_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

        SurfaceGeometry::render_gui();

        ImGui::Text("Radius");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Radius", &radius, 0.01f, 0.01f, 2.0f)) {
            dirty = true;
        }

        ImGui::Text("Height");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Height", &height, 0.01f, 0.01f, 2.0f)) {
            dirty = true;
        }

        ImGui::Text("Rings");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        int rings_int = static_cast<int>(rings);
        if (ImGui::DragInt("##Rings", &rings_int, 1, 3, 64)) {
            rings = static_cast<uint32_t>(rings_int);
            dirty = true;
        }

        ImGui::Text("Ring Segments");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        int ring_seg_int = static_cast<int>(ring_segments);
        if (ImGui::DragInt("##Ring Segments", &ring_seg_int, 1, 3, 64)) {
            ring_segments = static_cast<uint32_t>(ring_seg_int);
            dirty = true;
        }

        ImGui::Text("Capped");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::Checkbox("##Capped", &capped)) {
            dirty = true;
        }

        ImGui::TreePop();
    }

    if (dirty) {
        build_mesh();
        dirty = false;
    }
}
