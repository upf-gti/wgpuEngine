#include "parse_gltf.h"

#include "json.hpp"
#include "stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include <glm/gtx/quaternion.hpp>

#include "framework/utils/utils.h"
#include "graphics/texture.h"
#include "graphics/shader.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"

#include "spdlog/spdlog.h"


void create_material_texture(tinygltf::Model& model, int tex_index, Texture** texture) {

    const tinygltf::Texture& tex = model.textures[tex_index];

    if (tex.source < 0)
        return;

    tinygltf::Image& image = model.images[tex.source];

    assert(image.component == 4);

    WGPUTextureFormat texture_format = WGPUTextureFormat_RGBA8Unorm;

    switch (image.pixel_type) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        texture_format = WGPUTextureFormat_RGBA8Unorm;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        texture_format = WGPUTextureFormat_RGBA16Uint;
        break;
    default:
        assert(false);
    }

    bool convert_image = texture_format != WGPUTextureFormat_RGBA8Unorm;

    if (convert_image) {
        uint8_t* converted_texture = new uint8_t[image.width * image.height * 4];

        if (Texture::convert_to_rgba8unorm(image.width, image.height, texture_format, image.image.data(), converted_texture)) {
            *texture = new Texture();
            (*texture)->load_from_data(image.uri, image.width, image.height, converted_texture, WGPUTextureFormat_RGBA8Unorm);
            delete[] converted_texture;
        }
    }
    else {
        *texture = new Texture();
        (*texture)->load_from_data(image.uri, image.width, image.height, image.image.data(), WGPUTextureFormat_RGBA8Unorm);
    }

    if (tex.sampler != -1)
    {
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

}

void read_mesh(tinygltf::Model& model, tinygltf::Mesh& mesh, Entity* entity) {

    EntityMesh* entity_mesh = dynamic_cast<EntityMesh*>(entity);

    if (!entity_mesh) {
        assert(0);
        return;
    }

    glm::vec3 min_pos = { FLT_MAX, FLT_MAX, FLT_MAX };
    glm::vec3 max_pos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (size_t primitive_idx = 0; primitive_idx < mesh.primitives.size(); ++primitive_idx) {

        Surface* surface = new Surface();

        Material& material = surface->get_material();

        std::vector<InterleavedData>& vertices = surface->get_vertices();

        tinygltf::Primitive primitive = mesh.primitives[primitive_idx];

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
                    glm::vec3& position = vertices[vertex_idx].position;
                    position.x = *(float*)&buffer.data[buffer_idx + 0];
                    position.y = *(float*)&buffer.data[buffer_idx + 4];
                    position.z = *(float*)&buffer.data[buffer_idx + 8];

                    glm::bvec3 less_than = glm::lessThan(position, min_pos);

                    if (less_than.x) {
                        min_pos.x = position.x;
                    }
                    if (less_than.y) {
                        min_pos.y = position.y;
                    }
                    if (less_than.z) {
                        min_pos.z = position.z;
                    }

                    glm::bvec3 greater_than = glm::greaterThan(position, max_pos);

                    if (greater_than.x) {
                        max_pos.x = position.x;
                    }
                    if (greater_than.y) {
                        max_pos.y = position.y;
                    }
                    if (greater_than.z) {
                        max_pos.z = position.z;
                    }
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

        glm::vec3 aabb_half_size = (max_pos - min_pos) * 0.5f;
        glm::vec3 aabb_position = min_pos + aabb_half_size;

        entity_mesh->set_aabb({ aabb_position, aabb_half_size });

        material.flags |= MATERIAL_PBR;

        WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();
        bool is_openxr_available = Renderer::instance->get_openxr_available();
        WGPUTextureFormat swapchain_format = is_openxr_available ? webgpu_context->xr_swapchain_format : webgpu_context->swapchain_format;

        PipelineDescription description = {};

        WGPUColorTargetState color_target = {};
        color_target.format = swapchain_format;
        color_target.writeMask = WGPUColorWriteMask_All;

        std::vector<std::string> define_specializations;

        if (primitive.material >= 0) {

            tinygltf::Material& gltf_material = model.materials[primitive.material];

            const tinygltf::PbrMetallicRoughness& pbrMetallicRoughness = gltf_material.pbrMetallicRoughness;

            
            if (pbrMetallicRoughness.baseColorTexture.index >= 0) {
                create_material_texture(model, pbrMetallicRoughness.baseColorTexture.index, &material.diffuse_texture);
                material.flags |= MATERIAL_DIFFUSE;
                define_specializations.push_back("ALBEDO_TEXTURE");
            }

            material.color = glm::vec4(
                pbrMetallicRoughness.baseColorFactor[0],
                pbrMetallicRoughness.baseColorFactor[1],
                pbrMetallicRoughness.baseColorFactor[2],
                pbrMetallicRoughness.baseColorFactor[3]
            );

            if (pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
                create_material_texture(model, pbrMetallicRoughness.metallicRoughnessTexture.index, &material.metallic_roughness_texture);
                define_specializations.push_back("METALLIC_ROUGHNESS_TEXTURE");
            }
           
            material.roughness = static_cast<float>(pbrMetallicRoughness.roughnessFactor);
            material.metalness = static_cast<float>(pbrMetallicRoughness.metallicFactor);

            if (gltf_material.normalTexture.index >= 0) {
                define_specializations.push_back("NORMAL_TEXTURE");
                create_material_texture(model, gltf_material.normalTexture.index, &material.normal_texture);
            }

            if (gltf_material.emissiveTexture.index >= 0) {
                define_specializations.push_back("EMISSIVE_TEXTURE");
                create_material_texture(model, gltf_material.emissiveTexture.index, &material.emissive_texture);
            }

            material.emissive = { gltf_material.emissiveFactor[0], gltf_material.emissiveFactor[1], gltf_material.emissiveFactor[2] };

            if (!define_specializations.empty()) {
                define_specializations.push_back("USE_SAMPLER");
            }

            description.cull_mode = gltf_material.doubleSided ? WGPUCullMode_None : WGPUCullMode_Back;

            if (gltf_material.alphaMode == "OPAQUE") {
                define_specializations.push_back("ALPHA_OPAQUE");
            } else
            if (gltf_material.alphaMode == "BLEND") {

                WGPUBlendState* blend_state = new WGPUBlendState;
                blend_state->color = {
                        .operation = WGPUBlendOperation_Add,
                        .srcFactor = WGPUBlendFactor_SrcAlpha,
                        .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                };
                blend_state->alpha = {
                        .operation = WGPUBlendOperation_Add,
                        .srcFactor = WGPUBlendFactor_Zero,
                        .dstFactor = WGPUBlendFactor_One,
                };

                color_target.blend = blend_state;

                define_specializations.push_back("ALPHA_BLEND");

                material.flags |= MATERIAL_TRANSPARENT;
                description.uses_depth_write = false;
                description.blending_enabled = true;
            } else
            if (gltf_material.alphaMode == "MASK") {
                material.alpha_mask = static_cast<float>(gltf_material.alphaCutoff);
                material.flags |= MATERIAL_ALPHA_MASK;
                define_specializations.push_back("ALPHA_MASK");
            }
        }
        else {
            // create default material
            material.color = glm::vec4(1.0f);
            material.roughness = 1.0f;
            material.metalness = 0.0f;
        }

        material.shader = RendererStorage::get_shader("data/shaders/mesh_pbr.wgsl", define_specializations);

        Pipeline::register_render_pipeline(material.shader, color_target, description);

        surface->create_vertex_buffer();

        entity_mesh->add_surface(surface);
    }
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

bool parse_gltf(const std::string& gltf_path, std::vector<Entity*>& entities)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    std::filesystem::path path = std::filesystem::path(gltf_path);

    if (path.extension() == ".gltf")
    {
        if (!loader.LoadASCIIFromFile(&model, &err, &warn, gltf_path)) {
            spdlog::error("Could not load: {}", gltf_path);
            return false;
        }
    }
    else
    {
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, gltf_path)) {
            spdlog::error("Could not load binary: {}", gltf_path);
            return false;
        }
    }

    if (!warn.empty()) {
        spdlog::warn(warn);
    }

    if (!err.empty()) {
        spdlog::error(err);
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    int entity_name_idx = 0;
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));

        tinygltf::Node node = model.nodes[scene.nodes[i]];

        Entity* entity = create_node_entity(node);

        parse_model_nodes(model, node, entity);

        if (entity->get_name().empty()) {
            entity->set_name(path.stem().string() + "_" + std::to_string(entity_name_idx));
            entity_name_idx++;
        }

        entities.push_back(entity);
    }

    return true;
}
