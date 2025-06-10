#include "torus_geometry.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

TorusGeometry::TorusGeometry(float ring_radius, float tube_radius, uint32_t rings, uint32_t ring_segments, const glm::vec3& color):
    ring_radius(ring_radius), tube_radius(tube_radius), rings(rings), ring_segments(ring_segments), color(color)
{
    create_torus(ring_radius, tube_radius, rings, ring_segments, color);
}

void TorusGeometry::render_gui()
{
    Surface::render_gui();

    std::string surface_name = name.empty() ? "" : (" (" + name + ")");
    if (ImGui::TreeNodeEx(("Geometry" + surface_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::Text("Ring radius");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Ring radius", &ring_radius, 0.01f, 0.01f, 2.0f)) {
            dirty = true;
        }

        ImGui::Text("Tube radius");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Tube radius", &tube_radius, 0.01f, 0.01f, 2.0f)) {
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

        ImGui::TreePop();
    }

    if (dirty) {
        create_torus(ring_radius, tube_radius, rings, ring_segments, color);
        dirty = false;
    }
}
