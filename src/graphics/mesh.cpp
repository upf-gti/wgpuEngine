#include "mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "utils.h"

#include "texture.h"

#include "shader.h"

WebGPUContext* Mesh::webgpu_context = nullptr;

std::map<std::string, Mesh*> Mesh::meshes;

bool Mesh::load(const std::string& mesh_path)
{
    tinyobj::ObjReaderConfig reader_config;
    //reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(mesh_path, reader_config)) {
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
                    vertex_data.uv.y = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
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

    if (!materials.empty()) {
        if (materials[0].diffuse_texname.empty()) {
            create_bind_group_color(Shader::get("data/shaders/mesh_color.wgsl"), 0);
            update_material_color(glm::vec3(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]));
        }
        else {
            diffuse = Texture::get("data/textures/" + materials[0].diffuse_texname);
            create_bind_group_texture(Shader::get("data/shaders/mesh_texture.wgsl"), 0);
        }
    }

    return true;
}

Mesh::~Mesh()
{
    if (vertices.empty()) return;

    vertices.clear();

    wgpuBufferDestroy(vertex_buffer);

    wgpuBindGroupRelease(bind_group);

    mesh_data_uniform.destroy();
}

Mesh* Mesh::get(const std::string& mesh_path)
{
    std::string name = mesh_path;

    // check if already loaded
    std::map<std::string, Mesh*>::iterator it = meshes.find(mesh_path);
    if (it != meshes.end())
        return it->second;

    Mesh* ms = new Mesh();
    ms->load(mesh_path);

    // register in map
    meshes[name] = ms;

    return ms;
}

void Mesh::create_vertex_buffer()
{
    vertex_buffer = webgpu_context->create_buffer(get_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data());
}

void Mesh::create_bind_group_color(Shader* shader, uint16_t bind_group_id)
{
    this->shader = shader;

    sUniformMeshData default_data;
    default_data.model = glm::mat4x4(1.0f);
    default_data.color = glm::vec4(1.0f);
    mesh_data_uniform.data = webgpu_context->create_buffer(sizeof(sUniformMeshData), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &default_data);
    mesh_data_uniform.binding = 0;
    mesh_data_uniform.buffer_size = sizeof(sUniformMeshData);

    std::vector<Uniform*> uniforms = { &mesh_data_uniform };

    bind_group = webgpu_context->create_bind_group(uniforms, shader, bind_group_id);
}

void Mesh::create_bind_group_texture(Shader* shader, uint16_t bind_group_id)
{
    this->shader = shader;

    sUniformMeshData default_data;
    default_data.model = glm::mat4x4(1.0f);
    default_data.color = glm::vec4(1.0f);
    mesh_data_uniform.data = webgpu_context->create_buffer(sizeof(sUniformMeshData), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &default_data);
    mesh_data_uniform.binding = 0;
    mesh_data_uniform.buffer_size = sizeof(sUniformMeshData);

    albedo_uniform.data = diffuse->get_view();
    albedo_uniform.binding = 1;

    sampler_uniform.data = webgpu_context->create_sampler(); // Using all default params
    sampler_uniform.binding = 2;

    std::vector<Uniform*> uniforms = { &mesh_data_uniform, &albedo_uniform, &sampler_uniform };

    bind_group = webgpu_context->create_bind_group(uniforms, shader, bind_group_id);
}

WGPUBuffer& Mesh::get_vertex_buffer()
{
    return vertex_buffer;
}

WGPUBindGroup& Mesh::get_bind_group()
{
    return bind_group;
}

void Mesh::create_quad(float w, float h, const glm::vec3& color)
{
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
    create_bind_group_color(Shader::get("data/shaders/mesh_color.wgsl"), 0);
}

void Mesh::create_from_vertices(const std::vector<InterleavedData>& _vertices)
{
    vertices = _vertices;
    create_vertex_buffer();
}

void Mesh::update_model_matrix(const glm::mat4x4& model)
{
    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(mesh_data_uniform.data), 0, &model, sizeof(glm::mat4x4));
}

void Mesh::update_material_color(const glm::vec3& color)
{
    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(mesh_data_uniform.data), sizeof(glm::mat4x4), &color, sizeof(glm::vec3));
}

Shader* Mesh::get_shader()
{
    return shader;
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
