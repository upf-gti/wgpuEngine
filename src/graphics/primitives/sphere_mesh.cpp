#include "sphere_mesh.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

SphereMesh::SphereMesh(float radius, uint32_t rings, uint32_t ring_segments, const glm::vec3& color)
    : PrimitiveMesh(color), radius(radius), rings(rings), ring_segments(ring_segments)
{
    mesh_type = "SphereMesh";

    build_mesh();
}

void SphereMesh::build_mesh()
{
    surface->create_sphere(radius, ring_segments, rings, color);
}

void SphereMesh::set_radius(float new_radius)
{
    radius = new_radius;
    build_mesh();
}

void SphereMesh::set_rings(uint32_t new_rings)
{
    rings = new_rings;
    build_mesh();
}

void SphereMesh::set_ring_segments(uint32_t new_ring_segments)
{
    ring_segments = new_ring_segments;
    build_mesh();
}

void SphereMesh::render_gui()
{
    ImGui::Text("Radius");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::DragFloat("##Radius", &radius, 0.01f, 0.01f, 2.0f)) {
        dirty = true;
    }

    ImGui::Text("Rings");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    int rings_int = static_cast<int>(rings);
    if (ImGui::DragInt("##Rings", &rings_int, 1, 1, 64)) {
        rings = static_cast<uint32_t>(rings_int);
        dirty = true;
    }

    ImGui::Text("Ring Segments");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    int ring_seg_int = static_cast<int>(ring_segments);
    if (ImGui::DragInt("##Ring Segments", &ring_seg_int, 1, 4, 64)) {
        ring_segments = static_cast<uint32_t>(ring_seg_int);
        dirty = true;
    }

    if (dirty) {
        build_mesh();
        dirty = false;
    }

    PrimitiveMesh::render_gui();
}
