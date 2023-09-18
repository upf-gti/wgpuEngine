#include "mesh.h"

#include "utils.h"

#include "texture.h"

#include "shader.h"

WebGPUContext* Mesh::webgpu_context = nullptr;

std::map<std::string, Mesh*> Mesh::meshes;

Mesh::~Mesh()
{
    if (vertices.empty()) return;

    vertices.clear();

    wgpuBufferDestroy(vertex_buffer);

    if (bind_group) {
        wgpuBindGroupRelease(bind_group);
        mesh_data_uniform.destroy();
    }
}

Mesh* Mesh::get(const std::string& mesh_path)
{
    std::string name = mesh_path;

    // check if already loaded
    std::map<std::string, Mesh*>::iterator it = meshes.find(mesh_path);
    if (it != meshes.end())
        return it->second;

    Mesh* ms = new Mesh();

    //std::string extension = mesh_path.substr(mesh_path.find_last_of(".") + 1);

    //if (extension == "obj") {
    //    ms->load_obj(mesh_path);
    //}
    //else if (extension == "gltf") {
    //    ms->load_gltf(mesh_path);
    //}
    //else {
    //    std::cerr << "Mesh extension ." << extension << " not supported" << std::endl;
    //    assert(0);
    //}

    // register in map
    meshes[name] = ms;

    return ms;
}

void Mesh::create_vertex_buffer()
{
    vertex_buffer = webgpu_context->create_buffer(get_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data());
}

void Mesh::create_bind_group(Shader* shader, uint16_t bind_group_id, Texture* texture)
{
    if (bind_group) {
        wgpuBindGroupRelease(bind_group);
    }

    if (texture) {
        create_bind_group_texture(shader, bind_group_id, texture);
    }
    else {
        create_bind_group_color(shader, bind_group_id);
    }
}

void Mesh::create_bind_group_color(Shader* shader, uint16_t bind_group_id)
{
    uint32_t instances = static_cast<uint32_t>(instance_data.size());

    std::vector<sUniformMeshData> default_data = { instances, { glm::mat4x4(1.0f), glm::vec4(1.0f) } };

    mesh_data_uniform.data = webgpu_context->create_buffer(sizeof(sUniformMeshData) * instances, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, default_data.data());
    mesh_data_uniform.binding = 0;
    mesh_data_uniform.buffer_size = sizeof(sUniformMeshData) * instances;

    std::vector<Uniform*> uniforms = { &mesh_data_uniform };

    bind_group = webgpu_context->create_bind_group(uniforms, shader, bind_group_id);

    //for (uint32_t i = 0; i < instances; ++i)
    //{
    //    update_material_color(color, i);
    //}

    instances_gpu_size = instances;
}

void Mesh::create_bind_group_texture(Shader* shader, uint16_t bind_group_id, Texture* texture)
{
    uint32_t instances = static_cast<uint32_t>(instance_data.size());

    std::vector<sUniformMeshData> default_data = { instances, {glm::mat4x4(1.0f), glm::vec4(1.0f)}};

    mesh_data_uniform.data = webgpu_context->create_buffer(sizeof(sUniformMeshData) * instances, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, default_data.data());
    mesh_data_uniform.binding = 0;
    mesh_data_uniform.buffer_size = sizeof(sUniformMeshData) * instances;

    albedo_uniform.data = texture->get_view();
    albedo_uniform.binding = 1;

    sampler_uniform.data = webgpu_context->create_sampler(); // Using all default params
    sampler_uniform.binding = 2;

    std::vector<Uniform*> uniforms = { &mesh_data_uniform, &albedo_uniform, &sampler_uniform };

    bind_group = webgpu_context->create_bind_group(uniforms, shader, bind_group_id);

    instances_gpu_size = instances;
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
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
        wgpuBindGroupRelease(bind_group);
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

void Mesh::update_model_matrix(const glm::mat4x4& model, uint32_t instance_id)
{
    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(mesh_data_uniform.data), instance_id * sizeof(sUniformMeshData), &model, sizeof(glm::mat4x4));
}

void Mesh::update_material_color(const glm::vec3& color, uint32_t instance_id)
{
    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(mesh_data_uniform.data), instance_id * sizeof(sUniformMeshData) + sizeof(glm::mat4x4), &color, sizeof(glm::vec3));
}

void Mesh::update_instance_model_matrices()
{
    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(mesh_data_uniform.data), 0, instance_data.data(), instance_data.size() * sizeof(sUniformMeshData));
}

void Mesh::add_instance_data(sUniformMeshData model)
{
    instance_data.push_back(model);
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
