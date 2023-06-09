#include "mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "utils.h"

WebGPUContext* Mesh::webgpu_context = nullptr;

std::vector<WGPUVertexAttribute> Mesh::vertex_buffer_attributes[eVertexBufferLayout::VB_SIZE] = {};
WGPUVertexBufferLayout Mesh::vertex_buffer_layouts[eVertexBufferLayout::VB_SIZE] = {};

WGPUBindGroupLayout Mesh::bind_group_layouts[BG_SIZE] = {};

bool Mesh::vertex_buffer_layouts_initialized = false;
bool Mesh::bind_groups_initialized = false;

Uniform Mesh::default_uniform = {};

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
    create_bind_group();
}

Mesh::~Mesh()
{
    if (vertices.empty()) return;

    wgpuBufferDestroy(vertex_buffer);
}

void Mesh::init_vertex_buffer_layouts()
{
    std::vector<WGPUVertexAttribute> &default_attributes = vertex_buffer_attributes[VB_DEFAULT];
    default_attributes.resize(eVertexLayoutDefault::DEFAULT_SIZE);

    default_attributes[eVertexLayoutDefault::POSITION].shaderLocation = 0;
    default_attributes[eVertexLayoutDefault::POSITION].format = WGPUVertexFormat_Float32x3;
    default_attributes[eVertexLayoutDefault::POSITION].offset = 0;

    default_attributes[eVertexLayoutDefault::UV].shaderLocation = 1;
    default_attributes[eVertexLayoutDefault::UV].format = WGPUVertexFormat_Float32x2;
    default_attributes[eVertexLayoutDefault::UV].offset = sizeof(InterleavedData::position);

    default_attributes[eVertexLayoutDefault::NORMAL].shaderLocation = 2;
    default_attributes[eVertexLayoutDefault::NORMAL].format = WGPUVertexFormat_Float32x3;
    default_attributes[eVertexLayoutDefault::NORMAL].offset = sizeof(InterleavedData::position) + sizeof(InterleavedData::uv);

    default_attributes[eVertexLayoutDefault::COLOR].shaderLocation = 3;
    default_attributes[eVertexLayoutDefault::COLOR].format = WGPUVertexFormat_Float32x3;
    default_attributes[eVertexLayoutDefault::COLOR].offset = sizeof(InterleavedData::position) + sizeof(InterleavedData::uv) + sizeof(InterleavedData::normal);

    vertex_buffer_layouts[VB_DEFAULT] = webgpu_context->create_vertex_buffer_layout(default_attributes, sizeof(InterleavedData), WGPUVertexStepMode_Vertex);

    vertex_buffer_layouts_initialized = true;
}

void Mesh::init_bind_group_layouts()
{
    default_uniform.data = webgpu_context->create_buffer(sizeof(sUniformMeshData), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr);
    default_uniform.binding = 0;
    default_uniform.visibility = WGPUShaderStage_Vertex;
    default_uniform.buffer_size = sizeof(sUniformMeshData);

    bind_group_layouts[BG_DEFAULT] = webgpu_context->create_bind_group_layout( { &default_uniform } );

    bind_groups_initialized = true;
}

WGPUVertexBufferLayout Mesh::get_vertex_buffer_layout(eVertexBufferLayout layout_type)
{
    if (!vertex_buffer_layouts_initialized) {
        init_vertex_buffer_layouts();
    }

    if (layout_type >= eVertexBufferLayout::VB_SIZE || layout_type < 0) {
        assert(0);
        std::cerr << "Vertex Buffer Layout " << layout_type << " does not exist" << std::endl;
        return {};
    }

    return vertex_buffer_layouts[layout_type];
}

WGPUBindGroupLayout Mesh::get_bind_group_layout(eBindGroupLayout bind_group_type)
{
    if (!bind_groups_initialized) {
        init_bind_group_layouts();
    }

    if (bind_group_type >= eBindGroupLayout::BG_SIZE || bind_group_type < 0) {
        assert(0);
        std::cerr << "Bind Group " << bind_group_type << " does not exist" << std::endl;
        return {};
    }

    return bind_group_layouts[bind_group_type];
}

void Mesh::create_vertex_buffer()
{
    vertex_buffer = webgpu_context->create_buffer(get_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data());
}

void Mesh::create_bind_group()
{
    glm::mat4x4 identity(1.0f);
    uniform.data = webgpu_context->create_buffer(sizeof(sUniformMeshData), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &identity);
    uniform.binding = 0;
    uniform.visibility = WGPUShaderStage_Vertex;
    uniform.buffer_size = sizeof(sUniformMeshData);

    std::vector<Uniform*> uniforms = { &uniform };

    bind_group = webgpu_context->create_bind_group(uniforms, Mesh::get_bind_group_layout(BG_DEFAULT));
}

WGPUBuffer& Mesh::get_vertex_buffer()
{
    return vertex_buffer;
}

WGPUBindGroup& Mesh::get_bind_group()
{
    return bind_group;
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
    create_bind_group();
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
