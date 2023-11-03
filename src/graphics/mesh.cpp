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

std::vector<InterleavedData> Mesh::generate_quad(float w, float h, const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color)
{
    InterleavedData points[4];

    glm::vec3 vX = get_perpendicular(normal);
    glm::vec3 vY = glm::cross(normal, vX);
    glm::vec3 delta1 = w * vX;
    glm::vec3 delta2 = h * vY;

    // Build one corner of the square
    glm::vec3 orig = -0.5f * w * vX + 0.5f * h * vY;
    uint32_t counter = 0;

    for (unsigned short i1 = 0; i1 <= 1; i1++)
        for (unsigned short i2 = 0; i2 <= 1; i2++)
        {
            auto vtx = &points[counter++];
            vtx->position = position + 2.f * (orig + float(i1) * delta1 - float(i2) * delta2);
            vtx->normal = normal;
            vtx->uv = glm::vec2(i2, i1);
        }

    std::vector<InterleavedData> vertices;
    vertices.resize(6);

    counter = 0;

    vertices[counter++] = points[2]; 
    vertices[counter++] = points[0];
    vertices[counter++] = points[1];

    vertices[counter++] = points[2];
    vertices[counter++] = points[1];
    vertices[counter++] = points[3];

    return vertices;
}

void Mesh::create_quad(float w, float h, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    vertices = generate_quad(w, h, glm::vec3(0.f), normals::pZ, color);

    create_vertex_buffer();
}

void Mesh::create_box(float w, float h, float d, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    auto pos_x = generate_quad(w, h, d * normals::pX, normals::pX, color);
    vertices.insert(vertices.end(), pos_x.begin(), pos_x.end());
    auto neg_x = generate_quad(w, h, d * normals::nX, normals::nX, color);
    vertices.insert(vertices.end(), neg_x.begin(), neg_x.end());

    auto pos_y = generate_quad(w, h, d * normals::pY, normals::pY, color);
    vertices.insert(vertices.end(), pos_y.begin(), pos_y.end());
    auto neg_y = generate_quad(w, h, d * normals::nY, normals::nY, color);
    vertices.insert(vertices.end(), neg_y.begin(), neg_y.end());

    auto pos_z = generate_quad(w, h, d * normals::pZ, normals::pZ, color);
    vertices.insert(vertices.end(), pos_z.begin(), pos_z.end());
    auto neg_z = generate_quad(w, h, d * normals::nZ, normals::nZ, color);
    vertices.insert(vertices.end(), neg_z.begin(), neg_z.end());

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
