#include "parse_gltf.h"

#include "json.hpp"
#include "stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include <glm/gtx/quaternion.hpp>

#include "utils.h"
#include "graphics/mesh.h"
#include "graphics/texture.h"
#include "graphics/shader.h"

#include "graphics/renderer_storage.h"

#include <iostream>

void parse_gltf(const std::string& gltf_path, std::vector<Entity*>& entities)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    if (!loader.LoadASCIIFromFile(&model, &err, &warn, gltf_path)) {
        spdlog::error("Could not load: {}", gltf_path);
        return;
    }

    if (!warn.empty()) {
        spdlog::warn(warn);
    }

    if (!err.empty()) {
        spdlog::error(err);
    }

    std::function<void(tinygltf::Model&, tinygltf::Mesh&, Entity* entity)> read_mesh;
    std::function<void(tinygltf::Model&, tinygltf::Node&, Entity* parent_entity)> parse_model_nodes;
    std::function<Entity* (tinygltf::Node&)> create_node_entity;

    read_mesh = [&](tinygltf::Model& model, tinygltf::Mesh& mesh, Entity* entity) {

        EntityMesh* entity_mesh = dynamic_cast<EntityMesh*>(entity);

        if (!entity_mesh) {
            assert(0);
            return;
        }

        Mesh* new_mesh = new Mesh();
        auto& vertices = new_mesh->get_vertices();
        entity_mesh->set_mesh(new_mesh);

        Material& material = entity_mesh->get_material();

        for (size_t primitive_idx = 0; primitive_idx < mesh.primitives.size(); ++primitive_idx) {
            tinygltf::Primitive primitive = mesh.primitives[primitive_idx];

            bool uses_indices = primitive.indices >= 0;

            tinygltf::BufferView const* final_buffer_view = nullptr;
            tinygltf::BufferView const* indices_buffer_view = nullptr;
            tinygltf::Accessor const* final_accessor = nullptr;
            tinygltf::Accessor const* index_accessor = nullptr;
            tinygltf::Buffer const* indices_buffer = nullptr;

            unsigned int final_data_size = 0;
            unsigned int index_data_size = 0;

            if (uses_indices) {
                index_accessor = &model.accessors[primitive.indices];
                final_accessor = index_accessor;

                indices_buffer_view = &model.bufferViews[index_accessor->bufferView];
                indices_buffer = &model.buffers[indices_buffer_view->buffer];
                size_t index_buffer_size = 0;

                final_buffer_view = indices_buffer_view;

                int index_coponent_size = 1;

                switch (index_accessor->componentType) {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    index_data_size = index_coponent_size * sizeof(unsigned char);
                    index_buffer_size = indices_buffer_view->byteLength / index_data_size;
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index_data_size = index_coponent_size * sizeof(unsigned short);
                    index_buffer_size = indices_buffer_view->byteLength / index_data_size;
                    break;
                case TINYGLTF_COMPONENT_TYPE_INT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index_data_size = index_coponent_size * sizeof(unsigned int);
                    index_buffer_size = indices_buffer_view->byteLength / index_data_size;
                    break;
                }

                final_data_size = index_data_size;

                vertices.resize(index_buffer_size);
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

                if (!uses_indices) {
                    vertices.resize(buffer_size);
                    final_data_size = vertex_data_size;
                    final_buffer_view = &buffer_view;
                    final_accessor = &accessor;
                }

                int vertex_idx = 0;
                int final_stride;

                if (final_buffer_view->byteStride == 0) {
                    final_stride = final_data_size;
                }
                else {
                    final_stride = final_buffer_view->byteStride;
                }

                for (int j = final_accessor->byteOffset; j < final_buffer_view->byteLength; j += final_stride) {

                    size_t buffer_idx = 0;

                    if (uses_indices) {
                        size_t index_buffer_idx = j + indices_buffer_view->byteOffset;

                        int index = 0;
                        switch (index_accessor->componentType) {
                        case TINYGLTF_COMPONENT_TYPE_BYTE:
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            index = indices_buffer->data[index_buffer_idx];
                            break;
                        case TINYGLTF_COMPONENT_TYPE_SHORT:
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            index = *(unsigned short*)&(indices_buffer->data[index_buffer_idx]);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_INT:
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            index = *(unsigned int*)&(indices_buffer->data[index_buffer_idx]);
                            break;
                        }

                        int stride;

                        if (buffer_view.byteStride == 0) {
                            stride = vertex_data_size;
                        }
                        else {
                            stride = buffer_view.byteStride;
                        }

                        buffer_idx = index * stride + buffer_view.byteOffset + accessor.byteOffset;
                    }
                    else {
                        buffer_idx = j + buffer_view.byteOffset;
                    }

                    // position
                    if (attrib.first[0] == 'P') {
                        vertices[vertex_idx].position.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].position.y = *(float*)&buffer.data[buffer_idx + 4];
                        vertices[vertex_idx].position.z = *(float*)&buffer.data[buffer_idx + 8];
                    }

                    // normal
                    if (attrib.first[0] == 'N') {
                        vertices[vertex_idx].normal.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].normal.y = *(float*)&buffer.data[buffer_idx + 4];
                        vertices[vertex_idx].normal.z = *(float*)&buffer.data[buffer_idx + 8];
                    }

                    // uv
                    if (attrib.first[0] == 'T' && attrib.first[1] == 'E') {
                        vertices[vertex_idx].uv.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].uv.y = *(float*)&buffer.data[buffer_idx + 4];
                    }

                    vertex_idx++;
                }
            }

            if (primitive.material >= 0) {

                tinygltf::Material& gltf_material = model.materials[primitive.material];

                const tinygltf::PbrMetallicRoughness& pbrMetallicRoughness = gltf_material.pbrMetallicRoughness;

                if (pbrMetallicRoughness.baseColorTexture.index >= 0) {

                    tinygltf::Texture& tex = model.textures[pbrMetallicRoughness.baseColorTexture.index];

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
                        material.diffuse->load_from_data(image.uri, image.width, image.height, image.image.data());
                        material.shader = RendererStorage::get_shader("data/shaders/mesh_texture.wgsl");
                        material.flags |= MATERIAL_DIFFUSE;
                    }
                }
                else {
                    material.color = glm::vec4(
                        pbrMetallicRoughness.baseColorFactor[0],
                        pbrMetallicRoughness.baseColorFactor[1],
                        pbrMetallicRoughness.baseColorFactor[2],
                        pbrMetallicRoughness.baseColorFactor[3]
                    );

                    material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl");
                    material.flags |= MATERIAL_COLOR;
                }
            }
        }

        new_mesh->create_vertex_buffer();
    };

    create_node_entity = [](tinygltf::Node& node) {

        Entity* new_entity = nullptr;

        if (node.mesh >= 0) {
            new_entity = new EntityMesh();
        }
        else {
            new_entity = new Entity();
        }

        return new_entity;
    };

    parse_model_nodes = [&](tinygltf::Model& model, tinygltf::Node& node, Entity* entity) {
  
        if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
            read_mesh(model, model.meshes[node.mesh], entity);
        }

        // Set model matrix
        if (!node.matrix.empty())
        {
            glm::mat4x4 model_matrix;

            for (int j = 0; j < 16; ++j) {
                model_matrix[(j / 4) % 4][j % 4] = static_cast<float>(node.matrix[j]);
            }

            entity->set_model(model_matrix);
        }
        else {
            glm::mat4x4 translation_matrix = glm::mat4x4(1.0f);
            glm::quat rotation_quat = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
            glm::mat4x4 scale_matrix = glm::mat4x4(1.0f);

            if (!node.translation.empty()) {
                translation_matrix = glm::translate(glm::mat4x4(1.0f), {
                    static_cast<float>(node.translation[0]),
                    static_cast<float>(node.translation[1]),
                    static_cast<float>(node.translation[2])
                });
            }
            if (!node.rotation.empty()) {
                rotation_quat = glm::quat(
                    static_cast<float>(node.rotation[0]),
                    static_cast<float>(node.rotation[1]),
                    static_cast<float>(node.rotation[2]),
                    static_cast<float>(node.rotation[3])
                );
            }
            if (!node.scale.empty()) {
                scale_matrix = glm::scale(glm::mat4x4(1.0f), {
                    static_cast<float>(node.scale[0]),
                    static_cast<float>(node.scale[1]),
                    static_cast<float>(node.scale[2])
                });
            }

            entity->set_model(translation_matrix * glm::toMat4(rotation_quat) * scale_matrix);
        }

        // Parse children
        for (size_t i = 0; i < node.children.size(); i++) {
            assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
            Entity* child_entity = create_node_entity(model.nodes[node.children[i]]);
            entity->add_child(child_entity);
            parse_model_nodes(model, model.nodes[node.children[i]], child_entity);
        }
    };

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));

        tinygltf::Node node = model.nodes[scene.nodes[i]];

        Entity* entity = create_node_entity(node);

        parse_model_nodes(model, node, entity);

        entities.push_back(entity);
    }
}
