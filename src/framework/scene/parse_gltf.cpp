#include "parse_gltf.h"

#include "json.hpp"
#include "stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "utils.h"
#include "graphics/mesh.h"
#include "graphics/texture.h"
#include "graphics/shader.h"

#include "graphics/renderer_storage.h"

#include <iostream>

EntityMesh* parse_gltf(const std::string& gltf_path)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    if (!loader.LoadASCIIFromFile(&model, &err, &warn, gltf_path)) {
        std::cerr << "Could not load: " << gltf_path << std::endl;
        return nullptr;
    }

    if (!warn.empty()) {
        std::cout << "GLTF Load Warning: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cout << "GLTF Load Error: " << err << std::endl;
    }

    EntityMesh* new_entity = new EntityMesh();
    Mesh* new_mesh = new Mesh();
    auto& vertices = new_mesh->get_vertices();
    new_entity->set_mesh(new_mesh);

    Material& material = new_entity->get_material();

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

            vertices.resize(vertices.size() + index_buffer_size);

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
                        index = *(unsigned short*)&(indices_buffer.data[buffer_idx + 0]);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_INT:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        index = *(unsigned int*)&(indices_buffer.data[buffer_idx + 0]);
                        break;
                    }
       
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

                    vertex_idx++;
                }
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

                material.diffuse = new Texture();
                material.diffuse->load_from_data(image.name, image.width, image.height, image.image.data());
                material.shader = RendererStorage::get_shader("data/shaders/mesh_texture.wgsl");
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

    new_mesh->create_vertex_buffer();

    return new_entity;
}
