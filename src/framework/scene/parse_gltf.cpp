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

void create_material_texture(tinygltf::Model& model, int tex_index, Texture** texture) {

    const tinygltf::Texture& tex = model.textures[tex_index];

    if (tex.source < 0)
        return;

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

    *texture = new Texture();
    (*texture)->load_from_data(image.uri, image.width, image.height, image.image.data());

    tinygltf::Sampler& sampler = model.samplers[tex.sampler];

    switch (sampler.wrapS) {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        (*texture)->set_wrap_u(WGPUAddressMode_Repeat);
        break;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        (*texture)->set_wrap_u(WGPUAddressMode_MirrorRepeat);
        break;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        (*texture)->set_wrap_u(WGPUAddressMode_ClampToEdge);
        break;
    }

    switch (sampler.wrapT) {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        (*texture)->set_wrap_v(WGPUAddressMode_Repeat);
        break;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        (*texture)->set_wrap_v(WGPUAddressMode_MirrorRepeat);
        break;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        (*texture)->set_wrap_v(WGPUAddressMode_ClampToEdge);
        break;
    }
}

void read_mesh(tinygltf::Model& model, tinygltf::Mesh& mesh, Entity* entity) {

    EntityMesh* entity_mesh = dynamic_cast<EntityMesh*>(entity);

    if (!entity_mesh) {
        assert(0);
        return;
    }

    Mesh* new_mesh = new Mesh();
    std::vector<InterleavedData>& vertices = new_mesh->get_vertices();
    entity_mesh->set_mesh(new_mesh);

    Material& material = entity_mesh->get_material();

    for (size_t primitive_idx = 0; primitive_idx < mesh.primitives.size(); ++primitive_idx) {
        tinygltf::Primitive primitive = mesh.primitives[primitive_idx];

        assert(mesh.primitives.size() == 1);
        assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);

        bool uses_indices = primitive.indices >= 0;

        tinygltf::BufferView const* final_buffer_view = nullptr;
        tinygltf::BufferView const* indices_buffer_view = nullptr;
        tinygltf::Accessor const* final_accessor = nullptr;
        tinygltf::Accessor const* index_accessor = nullptr;
        tinygltf::Buffer const* indices_buffer = nullptr;

        size_t final_data_size = 0;
        size_t index_data_size = 0;

        if (uses_indices) {
            index_accessor = &model.accessors[primitive.indices];
            final_accessor = index_accessor;

            indices_buffer_view = &model.bufferViews[index_accessor->bufferView];
            indices_buffer = &model.buffers[indices_buffer_view->buffer];
            size_t index_buffer_size = 0;

            final_buffer_view = indices_buffer_view;

            uint16_t index_coponent_size = 1;

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
        }

        for (auto& attrib : primitive.attributes) {
            tinygltf::Accessor accessor = model.accessors[attrib.second];

            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

            const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

            uint16_t coponent_size = 1;
            if (accessor.type != TINYGLTF_TYPE_SCALAR) {
                coponent_size = accessor.type;
            }

            uint32_t vertex_data_size = coponent_size * sizeof(float);

            if (!uses_indices) {
                size_t buffer_size = buffer_view.byteLength / vertex_data_size;
                final_data_size = vertex_data_size;
                final_buffer_view = &buffer_view;
                final_accessor = &accessor;
            }

            size_t vertex_idx = 0;
            size_t final_stride;

            if (final_buffer_view->byteStride == 0) {
                final_stride = final_data_size;
            }
            else {
                final_stride = final_buffer_view->byteStride;
            }

            size_t buffer_start = final_accessor->byteOffset + final_buffer_view->byteOffset;
            size_t buffer_end = final_data_size * final_accessor->count + buffer_start;

            vertices.resize((buffer_end - buffer_start) / final_stride);

            for (size_t j = buffer_start; j < buffer_end; j += final_stride) {

                size_t buffer_idx = 0;

                if (uses_indices) {

                    uint32_t index = 0;
                    switch (index_accessor->componentType) {
                    //case TINYGLTF_COMPONENT_TYPE_BYTE:
                    //    index = *(int8_t*)&(indices_buffer->data[j]);
                    //    break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        index = *(uint8_t*)&(indices_buffer->data[j]);
                        break;
                    //case TINYGLTF_COMPONENT_TYPE_SHORT:
                    //    index = *(int16_t*)&(indices_buffer->data[j]);
                    //    break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        index = *(uint16_t*)&(indices_buffer->data[j]);
                        break;
                    //case TINYGLTF_COMPONENT_TYPE_INT:
                    //    index = *(int32_t*)&(indices_buffer->data[j]);
                    //    break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        index = *(uint32_t*)&(indices_buffer->data[j]);
                        break;
                    default:
                        assert(0);
                    }

                    size_t stride;

                    if (buffer_view.byteStride == 0) {
                        stride = vertex_data_size;
                    }
                    else {
                        stride = buffer_view.byteStride;
                    }

                    buffer_idx = index * stride + buffer_view.byteOffset + accessor.byteOffset;
                }
                else {
                    buffer_idx = j;
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

                create_material_texture(model, pbrMetallicRoughness.baseColorTexture.index, &material.diffuse);
                create_material_texture(model, pbrMetallicRoughness.metallicRoughnessTexture.index, &material.metallic_roughness);

                material.shader = RendererStorage::get_shader("data/shaders/mesh_texture.wgsl");
                /*material.metallic_roughness ?
                    RendererStorage::get_shader("data/shaders/mesh_pbr.wgsl") :
                    RendererStorage::get_shader("data/shaders/mesh_texture.wgsl");*/
                material.flags |= MATERIAL_DIFFUSE;
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

Entity* create_node_entity(tinygltf::Node& node) {

    Entity* new_entity = nullptr;

    if (node.mesh >= 0) {
        new_entity = new EntityMesh();
    }
    else {
        new_entity = new Entity();
    }

    return new_entity;
};

void parse_model_nodes(tinygltf::Model& model, tinygltf::Node& node, Entity* entity) {

    entity->set_name(node.name);

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

void parse_gltf(const std::string& gltf_path, std::vector<Entity*>& entities)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    std::string extension = gltf_path.substr(gltf_path.find_last_of(".") + 1);

    if (extension == "gltf")
    {
        if (!loader.LoadASCIIFromFile(&model, &err, &warn, gltf_path)) {
            spdlog::error("Could not load: {}", gltf_path);
            return;
        }
    }
    else
    {
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, gltf_path)) {
            spdlog::error("Could not load binary: {}", gltf_path);
            return;
        }
    }

    if (!warn.empty()) {
        spdlog::warn(warn);
    }

    if (!err.empty()) {
        spdlog::error(err);
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));

        tinygltf::Node node = model.nodes[scene.nodes[i]];

        Entity* entity = create_node_entity(node);

        parse_model_nodes(model, node, entity);

        entities.push_back(entity);
    }
}
