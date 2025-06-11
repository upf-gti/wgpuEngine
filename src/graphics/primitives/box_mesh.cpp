#include "box_mesh.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"
#include "imgui.h"

BoxMesh::BoxMesh(float width, float height, float depth, const glm::vec3& color)
    : PrimitiveMesh(color), width(width), height(height), depth(depth)
{
    mesh_type = "BoxMesh";

    build_mesh();
}

void BoxMesh::build_mesh()
{
    PrimitiveMesh::build_mesh();

    surface->create_box(width, height, depth, color);
}

void BoxMesh::set_width(float new_width)
{
    width = new_width;
    build_mesh();
}

void BoxMesh::set_height(float new_height)
{
    height = new_height;
    build_mesh();
}

void BoxMesh::set_depth(float new_depth)
{
    depth = new_depth;
    build_mesh();
}

void BoxMesh::set_size(const glm::vec3& size)
{
    width = size.x;
    height = size.y;
    depth = size.z;
    build_mesh();
}

void BoxMesh::render_gui()
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

    ImGui::Text("Depth");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::DragFloat("##Depth", &depth, 0.01f, 0.01f, 2.0f)) {
        dirty = true;
    }

    if (dirty) {
        build_mesh();
        dirty = false;
    }

    PrimitiveMesh::render_gui();
}
