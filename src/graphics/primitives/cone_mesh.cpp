#include "cone_mesh.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

ConeMesh::ConeMesh(float radius, float height, uint32_t segments, const glm::vec3& color)
    : PrimitiveMesh(color), radius(radius), height(height), segments(segments)
{
    mesh_type = "ConeMesh";

    build_mesh();
}

void ConeMesh::build_mesh()
{
    surface->create_cone(radius, height, segments, color);
}

void ConeMesh::set_radius(float new_radius)
{
    radius = new_radius;
    build_mesh();
}

void ConeMesh::set_height(float new_height)
{
    height = new_height;
    build_mesh();
}

void ConeMesh::set_segments(uint32_t new_segments)
{
    segments = new_segments;
    build_mesh();
}

void ConeMesh::render_gui()
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

    ImGui::Text("Segments");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    int seg_int = static_cast<int>(segments);
    if (ImGui::DragInt("##Segments", &seg_int, 1, 3, 64)) {
        segments = static_cast<uint32_t>(seg_int);
        dirty = true;
    }

    if (dirty) {
        build_mesh();
        dirty = false;
    }

    PrimitiveMesh::render_gui();
}
