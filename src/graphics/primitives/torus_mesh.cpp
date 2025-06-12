#include "torus_mesh.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

TorusMesh::TorusMesh(float ring_radius, float tube_radius, uint32_t rings, uint32_t ring_segments, const glm::vec3& color)
    : PrimitiveMesh(color), ring_radius(ring_radius), tube_radius(tube_radius), rings(rings), ring_segments(ring_segments)
{
    mesh_type = "TorusMesh";

    build_mesh();
}

void TorusMesh::build_mesh()
{
    surface->create_torus(ring_radius, tube_radius, rings, ring_segments, color);
}

void TorusMesh::set_ring_radius(float new_ring_radius)
{
    ring_radius = new_ring_radius;
    build_mesh();
}

void TorusMesh::set_tube_radius(float new_tube_radius)
{
    tube_radius = new_tube_radius;
    build_mesh();
}

void TorusMesh::set_rings(uint32_t new_rings)
{
    rings = new_rings;
    build_mesh();
}

void TorusMesh::set_ring_segments(uint32_t new_ring_segments)
{
    ring_segments = new_ring_segments;
    build_mesh();
}

void TorusMesh::render_gui()
{
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

    if (dirty) {
        build_mesh();
        dirty = false;
    }

    PrimitiveMesh::render_gui();
}
