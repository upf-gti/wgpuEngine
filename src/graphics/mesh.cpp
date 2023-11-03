#include "mesh.h"

#include "utils.h"

#include "texture.h"

#include "shader.h"

WebGPUContext* Mesh::webgpu_context = nullptr;
Mesh* Mesh::quad_mesh = nullptr;

Mesh::~Mesh()
{
    if (vertices.empty()) return;

    vertices.clear();

    wgpuBufferDestroy(vertex_buffer);
}

void Mesh::create_vertex_buffer()
{
    vertex_buffer = webgpu_context->create_buffer(get_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data(), "mesh_buffer");
}

WGPUBuffer& Mesh::get_vertex_buffer()
{
    return vertex_buffer;
}

void Mesh::create_quad(float w, float h, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    vertices.resize(6);
    
    vertices[0].position = { -w,  h, 0.f };
    vertices[1].position = { -w, -h, 0.f };
    vertices[2].position = {  w, -h, 0.f };

    vertices[3].position = { -w,  h, 0.f };
    vertices[4].position = {  w, -h, 0.f };
    vertices[5].position = {  w,  h, 0.f };

    vertices[0].uv = { 0.f, 1.f };
    vertices[1].uv = { 0.f, 0.f };
    vertices[2].uv = { 1.f, 0.f };

    vertices[3].uv = { 0.f, 1.f };
    vertices[4].uv = { 1.f, 0.f };
    vertices[5].uv = { 1.f, 1.f };

    vertices[0].normal = { 0.f, 1.f, 0.f };
    vertices[1].normal = { 0.f, 1.f, 0.f };
    vertices[2].normal = { 0.f, 1.f, 0.f };

    vertices[3].normal = { 0.f, 1.f, 0.f };
    vertices[4].normal = { 0.f, 1.f, 0.f };
    vertices[5].normal = { 0.f, 1.f, 0.f };

    vertices[0].color = color;
    vertices[1].color = color;
    vertices[2].color = color;

    vertices[3].color = color;
    vertices[4].color = color;
    vertices[5].color = color;

    create_vertex_buffer();
}

void Mesh::create_from_vertices(const std::vector<InterleavedData>& _vertices)
{
    vertices = _vertices;
    create_vertex_buffer();
}

void* Mesh::data()
{
    return vertices.data();
}

uint32_t Mesh::get_vertex_count()
{
    return static_cast<uint32_t>(vertices.size());
}

uint64_t Mesh::get_byte_size()
{
    return get_vertex_count() * sizeof(InterleavedData);
}
