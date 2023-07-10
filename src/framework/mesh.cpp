#include "mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "utils.h"

WebGPUContext* Mesh::webgpu_context = nullptr;

bool Mesh::load_mesh(const char* filepath)
{
    tinyobj::ObjReaderConfig reader_config;
    //reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filepath, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    InterleavedData vertex_data;

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {

                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                vertex_data.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                vertex_data.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                vertex_data.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                if (idx.normal_index >= 0) {
                    vertex_data.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex_data.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex_data.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                if (idx.texcoord_index >= 0) {
                    vertex_data.uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex_data.uv.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }

                vertex_data.color.x = attrib.colors[3*size_t(idx.vertex_index) + 0];
                vertex_data.color.y = attrib.colors[3*size_t(idx.vertex_index) + 1];
                vertex_data.color.z = attrib.colors[3*size_t(idx.vertex_index) + 2];

                vertices.push_back(vertex_data);
            }

            index_offset += fv;
        }
    }

    create_vertex_buffer();
}

Mesh::~Mesh()
{
    if (vertices.empty()) return;

    wgpuBufferDestroy(vertex_buffer);
}

void Mesh::create_vertex_buffer()
{
    assert(vertex_attributes.empty());

    vertex_attributes.resize(4);

    vertex_attributes[POSITION].shaderLocation = 0;
    vertex_attributes[POSITION].format = WGPUVertexFormat_Float32x3;
    vertex_attributes[POSITION].offset = 0;

    vertex_attributes[UV].shaderLocation = 1;
    vertex_attributes[UV].format = WGPUVertexFormat_Float32x2;
    vertex_attributes[UV].offset = sizeof(InterleavedData::position);

    vertex_attributes[NORMAL].shaderLocation = 2;
    vertex_attributes[NORMAL].format = WGPUVertexFormat_Float32x3;
    vertex_attributes[NORMAL].offset = sizeof(InterleavedData::position) + sizeof(InterleavedData::uv);

    vertex_attributes[COLOR].shaderLocation = 3;
    vertex_attributes[COLOR].format = WGPUVertexFormat_Float32x3;
    vertex_attributes[COLOR].offset = sizeof(InterleavedData::position) + sizeof(InterleavedData::uv) + sizeof(InterleavedData::normal);

    vertex_buffer_layout = webgpu_context->create_vertex_buffer_layout(vertex_attributes, sizeof(InterleavedData), WGPUVertexStepMode_Vertex);

    vertex_buffer = webgpu_context->create_buffer(get_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data());
}

std::vector<WGPUVertexAttribute>& Mesh::get_vertex_attributes()
{
    return vertex_attributes;
}

WGPUVertexBufferLayout& Mesh::get_vertex_buffer_layout()
{
    return vertex_buffer_layout;
}

WGPUBuffer& Mesh::get_vertex_buffer()
{
    return vertex_buffer;
}

void Mesh::create_quad()
{
    vertices.resize(6);

    vertices[0].position = { -1.0, 1.0, 0.0 };
    vertices[1].position = { -1.0,-1.0, 0.0 };
    vertices[2].position = {  1.0,-1.0, 0.0 };

    vertices[3].position = { -1.0, 1.0, 0.0 };
    vertices[4].position = {  1.0,-1.0, 0.0 };
    vertices[5].position = {  1.0, 1.0, 0.0 };

    vertices[0].uv = { 0.0, 1.0 };
    vertices[1].uv = { 0.0, 0.0 };
    vertices[2].uv = { 1.0, 0.0 };

    vertices[3].uv = { 0.0, 1.0 };
    vertices[4].uv = { 1.0, 0.0 };
    vertices[5].uv = { 1.0, 1.0 };

    create_vertex_buffer();
}

void* Mesh::data()
{
    return vertices.data();
}

size_t Mesh::get_vertex_count()
{
    return vertices.size();
}

size_t Mesh::get_byte_size()
{
    return get_vertex_count() * sizeof(InterleavedData);
}
