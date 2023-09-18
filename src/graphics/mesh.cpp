#include "mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "json.hpp"
#include "stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "utils.h"

#include "texture.h"

#include "shader.h"

WebGPUContext* Mesh::webgpu_context = nullptr;

std::map<std::string, Mesh*> Mesh::meshes;

bool Mesh::load_obj(const std::string& mesh_path)
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
            color = glm::vec4(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2], 1.0f);
        }
        else {
            diffuse = Texture::get("data/textures/" + materials[0].diffuse_texname);
        }
    }

    return true;
}

bool Mesh::load_gltf(const std::string& scene_path)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    if (!loader.LoadASCIIFromFile(&model, &err, &warn, scene_path)) {
        std::cerr << "Could not load: " << scene_path << std::endl;
        return false;
    }

    if (!warn.empty()) {
        std::cout << "GLTF Load Warning: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cout << "GLTF Load Error: " << err << std::endl;
    }

    std::function<void(tinygltf::Model&, tinygltf::Mesh&)> read_mesh;
    std::function<void(tinygltf::Model&, tinygltf::Node&)> parse_model_nodes;

    read_mesh = [&](tinygltf::Model& model, tinygltf::Mesh& mesh) {

        for (size_t primitive_idx = 0; primitive_idx < mesh.primitives.size(); ++primitive_idx) {
            tinygltf::Primitive primitive = mesh.primitives[primitive_idx];
            tinygltf::Accessor index_accessor = model.accessors[primitive.indices];

            const tinygltf::BufferView& indices_buffer_view = model.bufferViews[index_accessor.bufferView];
            const tinygltf::Buffer& indices_buffer = model.buffers[indices_buffer_view.buffer];
            size_t index_buffer_size = 0;

            int index_coponent_size = 1;

            unsigned int index_data_size = 0;

            switch (index_accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                index_data_size = index_coponent_size * sizeof(unsigned char);
                index_buffer_size = indices_buffer_view.byteLength / index_data_size;
                break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                index_data_size = index_coponent_size * sizeof(unsigned short);
                index_buffer_size = indices_buffer_view.byteLength / index_data_size;
                break;
            case TINYGLTF_COMPONENT_TYPE_INT:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                index_data_size = index_coponent_size * sizeof(unsigned int);
                index_buffer_size = indices_buffer_view.byteLength / index_data_size;
                break;
            }

            if (!vertices.empty()) {
                assert(0);
            }

            vertices.resize(index_buffer_size);

            int vertex_idx = 0;
            for (int j = 0; j < indices_buffer_view.byteLength; j += index_data_size) {
                int index = 0;
                size_t buffer_idx = j + indices_buffer_view.byteOffset;

                switch (index_accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    index = indices_buffer.data[buffer_idx];
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index = bytes_to_ushort(indices_buffer.data[buffer_idx + 0], indices_buffer.data[buffer_idx + 1]);
                    break;
                case TINYGLTF_COMPONENT_TYPE_INT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index = bytes_to_uint(indices_buffer.data[buffer_idx + 0], indices_buffer.data[buffer_idx + 1], indices_buffer.data[buffer_idx + 2], indices_buffer.data[buffer_idx + 3]);
                    break;
                }

                for (auto& attrib : primitive.attributes) {
                    tinygltf::Accessor accessor = model.accessors[attrib.second];

                    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

                    int coponent_size = 1;
                    if (accessor.type != TINYGLTF_TYPE_SCALAR) {
                        coponent_size = accessor.type;
                    }

                    unsigned int vertex_data_size = coponent_size * sizeof(float);

                    size_t buffer_size = buffer_view.byteLength / vertex_data_size;

                    // position
                    if (attrib.first[0] == 'P') {
                        size_t buffer_idx = index * vertex_data_size + buffer_view.byteOffset;
                        vertices[vertex_idx].position.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].position.y = *(float*)&buffer.data[buffer_idx + 4];
                        vertices[vertex_idx].position.z = *(float*)&buffer.data[buffer_idx + 8];
                    }

                    // normal
                    if (attrib.first[0] == 'N') {
                        size_t buffer_idx = index * vertex_data_size + buffer_view.byteOffset;
                        vertices[vertex_idx].normal.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].normal.y = *(float*)&buffer.data[buffer_idx + 4];
                        vertices[vertex_idx].normal.z = *(float*)&buffer.data[buffer_idx + 8];
                    }

                    // uv
                    if (attrib.first[0] == 'T') {
                        size_t buffer_idx = index * vertex_data_size + buffer_view.byteOffset;
                        vertices[vertex_idx].uv.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].uv.y = *(float*)&buffer.data[buffer_idx + 4];
                    }
                }
                vertex_idx++;
            }
        }

        if (model.textures.size() > 0) {

            tinygltf::Texture& tex = model.textures[0];

            if (tex.source > -1) {

                tinygltf::Image& image = model.images[tex.source];

                if (image.component == 1) {
                    // R
                }
                else if (image.component == 2) {
                    // RG
                }
                else if (image.component == 3) {
                    // RGB
                }
                else {

                }

                if (image.bits == 8) {
                }
                else if (image.bits == 16) {
                }
                else {
                    // ???
                }

                diffuse = new Texture();
                diffuse->load_from_data(image.name, image.width, image.height, image.image.data());
            }
        }
    };

    parse_model_nodes = [&](tinygltf::Model& model, tinygltf::Node& node) {
        if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
            read_mesh(model, model.meshes[node.mesh]);
        }

        for (size_t i = 0; i < node.children.size(); i++) {
            assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
            parse_model_nodes(model, model.nodes[node.children[i]]);
        }
    };

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
        parse_model_nodes(model, model.nodes[scene.nodes[i]]);
    }

    create_vertex_buffer();

    return false;
}

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

    std::string extension = mesh_path.substr(mesh_path.find_last_of(".") + 1);

    if (extension == "obj") {
        ms->load_obj(mesh_path);
    }
    else if (extension == "gltf") {
        ms->load_gltf(mesh_path);
    }
    else {
        std::cerr << "Mesh extension ." << extension << " not supported" << std::endl;
        assert(0);
    }

    // register in map
    meshes[name] = ms;

    return ms;
}

void Mesh::create_vertex_buffer()
{
    vertex_buffer = webgpu_context->create_buffer(get_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data());
}

void Mesh::create_bind_group(Shader* shader, uint16_t bind_group_id)
{
    if (bind_group) {
        wgpuBindGroupRelease(bind_group);
    }

    if (diffuse) {
        create_bind_group_texture(shader, bind_group_id);
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

    for (uint32_t i = 0; i < instances; ++i)
    {
        update_material_color(color, i);
    }

    instances_gpu_size = instances;
}

void Mesh::create_bind_group_texture(Shader* shader, uint16_t bind_group_id)
{
    uint32_t instances = static_cast<uint32_t>(instance_data.size());

    std::vector<sUniformMeshData> default_data = { instances, {glm::mat4x4(1.0f), glm::vec4(1.0f)}};

    mesh_data_uniform.data = webgpu_context->create_buffer(sizeof(sUniformMeshData) * instances, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, default_data.data());
    mesh_data_uniform.binding = 0;
    mesh_data_uniform.buffer_size = sizeof(sUniformMeshData) * instances;

    albedo_uniform.data = diffuse->get_view();
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
