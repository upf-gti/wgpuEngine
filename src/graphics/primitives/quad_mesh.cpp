#include "quad_mesh.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

QuadMesh::QuadMesh(float width, float height, bool flip_y, bool centered, uint32_t subdivisions, const glm::vec3& color)
    : PrimitiveMesh(color), width(width), height(height), flip_y(flip_y), centered(centered), subdivisions(subdivisions)
{
    mesh_type = "QuadMesh";

    build_mesh();
}

void QuadMesh::build_mesh()
{
    if (subdivisions > 0) {
        surface->create_subdivided_quad(width, height, flip_y, subdivisions, centered, color);
    }
    else {
        surface->create_quad(width, height, flip_y, centered, color);
    }
}

void QuadMesh::set_width(float new_width)
{
    width = new_width;
    build_mesh();
}

void QuadMesh::set_height(float new_height)
{
    height = new_height;
    build_mesh();
}

void QuadMesh::set_subdivisions(uint32_t new_subdivisions)
{
    subdivisions = new_subdivisions;
    build_mesh();
}

void QuadMesh::set_size(const glm::vec2& size)
{
    width = size.x;
    height = size.y;
    build_mesh();
}

void QuadMesh::set_centered(bool new_centered)
{
    centered = new_centered;
    build_mesh();
}

void QuadMesh::set_flip_y(bool new_flip_y)
{
    flip_y = new_flip_y;
    build_mesh();
}

void QuadMesh::render_gui()
{
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

    ImGui::Text("Flip Y");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::Checkbox("##Flip Y", &flip_y)) {
        dirty = true;
    }

    ImGui::Text("Centered");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::Checkbox("##Centered", &centered)) {
        dirty = true;
    }

    ImGui::Text("Subdivisions");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    int subdivisions_int = static_cast<int>(subdivisions);
    if (ImGui::DragInt("##Subdivisions", &subdivisions_int, 1, 0, 64)) {
        subdivisions = static_cast<uint32_t>(subdivisions_int);
        dirty = true;
    }

    if (dirty) {
        build_mesh();
        dirty = false;
    }

    PrimitiveMesh::render_gui();
}
