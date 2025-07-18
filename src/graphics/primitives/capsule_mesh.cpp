#include "capsule_mesh.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

CapsuleMesh::CapsuleMesh(float radius, float height, uint32_t rings, uint32_t ring_segments, const glm::vec3& color)
    : PrimitiveMesh(color), radius(radius), height(height), rings(rings), ring_segments(ring_segments)
{
    mesh_type = "CapsuleMesh";

    build_mesh();
}

void CapsuleMesh::build_mesh()
{
    surface->create_capsule(radius, height, rings, ring_segments, color);
}

void CapsuleMesh::set_radius(float new_radius)
{
    radius = new_radius;
    build_mesh();
}

void CapsuleMesh::set_height(float new_height)
{
    height = new_height;
    build_mesh();
}

void CapsuleMesh::set_rings(uint32_t new_rings)
{
    rings = new_rings;
    build_mesh();
}

void CapsuleMesh::set_ring_segments(uint32_t new_ring_segments)
{
    ring_segments = new_ring_segments;
    build_mesh();
}

void CapsuleMesh::render_gui()
{
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

    if (dirty) {
        build_mesh();
        dirty = false;
    }

    PrimitiveMesh::render_gui();
}
