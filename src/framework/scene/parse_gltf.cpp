#include "parse_gltf.h"

#include "framework/math.h"

#include "json.hpp"
#include "stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "framework/utils/utils.h"
#include "framework/nodes/mesh_instance_3d.h"
#include "framework/nodes/skeleton_instance_3d.h"
#include "framework/nodes/camera.h"

#include "graphics/texture.h"
#include "graphics/shader.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"

#include "spdlog/spdlog.h"


void create_material_texture(tinygltf::Model& model, int tex_index, Texture** texture, bool is_srgb = false) {

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
            (*texture)->load_from_data(image.uri, image.width, image.height, 1, converted_texture, true, is_srgb ? WGPUTextureFormat_RGBA8UnormSrgb : WGPUTextureFormat_RGBA8Unorm);
            delete[] converted_texture;
        }
    }
    else {
        *texture = new Texture();
        (*texture)->load_from_data(image.uri, image.width, image.height, 1, image.image.data(), true, is_srgb ? WGPUTextureFormat_RGBA8UnormSrgb : WGPUTextureFormat_RGBA8Unorm);
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

void read_mesh(tinygltf::Model& model, tinygltf::Mesh& mesh, Node3D* entity, std::map<uint32_t, Texture*>& texture_cache) {

    MeshInstance3D* entity_mesh = dynamic_cast<MeshInstance3D*>(entity);

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

            uint16_t component_size = 1;
            if (accessor.type != TINYGLTF_TYPE_SCALAR) {
                component_size = accessor.type;
            }

            uint32_t vertex_attribute_size;

            switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                vertex_attribute_size = component_size * sizeof(float);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                vertex_attribute_size = component_size * sizeof(uint8_t);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                vertex_attribute_size = component_size * sizeof(uint16_t);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                vertex_attribute_size = component_size * sizeof(uint32_t);
                break;
            default:
                assert(0);
            }

            if (!uses_indices) {
                size_t buffer_size = buffer_view.byteLength / vertex_attribute_size;
                final_data_size = vertex_attribute_size;
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
                        stride = vertex_attribute_size;
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
                if (attrib.first == std::string("TEXCOORD_0")) {
                    vertices[vertex_idx].uv.x = *(float*)&buffer.data[buffer_idx + 0];
                    vertices[vertex_idx].uv.y = *(float*)&buffer.data[buffer_idx + 4];
                }

                // tangents
                if (attrib.first[0] == 'T' && attrib.first[1] == 'A') {
                    vertices[vertex_idx].tangent.x = *(float*)&buffer.data[buffer_idx + 0];
                    vertices[vertex_idx].tangent.y = *(float*)&buffer.data[buffer_idx + 4];
                    vertices[vertex_idx].tangent.z = *(float*)&buffer.data[buffer_idx + 8];
                }

                // color
                if (attrib.first == std::string("COLOR_0")) {
                    switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        vertices[vertex_idx].color.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].color.y = *(float*)&buffer.data[buffer_idx + 4];
                        vertices[vertex_idx].color.z = *(float*)&buffer.data[buffer_idx + 8];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        vertices[vertex_idx].color.x = *(uint8_t*)&buffer.data[buffer_idx + 0] / 255.0f;
                        vertices[vertex_idx].color.y = *(uint8_t*)&buffer.data[buffer_idx + 1] / 255.0f;
                        vertices[vertex_idx].color.z = *(uint8_t*)&buffer.data[buffer_idx + 2] / 255.0f;
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        vertices[vertex_idx].color.x = *(uint16_t*)&buffer.data[buffer_idx + 0] / 65535.0f;
                        vertices[vertex_idx].color.y = *(uint16_t*)&buffer.data[buffer_idx + 2] / 65535.0f;
                        vertices[vertex_idx].color.z = *(uint16_t*)&buffer.data[buffer_idx + 4] / 65535.0f;
                        break;
                    default:
                        assert(0);
                    }
                }

                // joints (skinning)
                if (attrib.first == std::string("JOINTS_0")) {
                    glm::vec4 joints;

                    switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        joints.x = *(uint8_t*)&buffer.data[buffer_idx + 0];
                        joints.y = *(uint8_t*)&buffer.data[buffer_idx + 1];
                        joints.z = *(uint8_t*)&buffer.data[buffer_idx + 2];
                        joints.w = *(uint8_t*)&buffer.data[buffer_idx + 3];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        joints.x = *(uint16_t*)&buffer.data[buffer_idx + 0];
                        joints.y = *(uint16_t*)&buffer.data[buffer_idx + 2];
                        joints.z = *(uint16_t*)&buffer.data[buffer_idx + 4];
                        joints.w = *(uint16_t*)&buffer.data[buffer_idx + 8];
                        break;
                    default:
                        assert(0);
                    }

                    // TO DO: convert the joint indices so that they go from being relative to the joints array to being relative to the skeleton hierarchy
                    // 
                    //Make sure that even the invalid nodes have a value of 0 (any negative joint indices will break the skinning implementation)
                    joints.x = std::max(0.f, joints.x);
                    joints.y = std::max(0.f, joints.y);
                    joints.z = std::max(0.f, joints.z);
                    joints.w = std::max(0.f, joints.w);

                    vertices[vertex_idx].joints.x = joints.x;
                    vertices[vertex_idx].joints.y = joints.y;
                    vertices[vertex_idx].joints.z = joints.z;
                    vertices[vertex_idx].joints.w = joints.w;
                }

                // weights (skinning)
                if (attrib.first == std::string("WEIGHTS_0")) {
                    switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        vertices[vertex_idx].weights.x = *(float*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].weights.y = *(float*)&buffer.data[buffer_idx + 4];
                        vertices[vertex_idx].weights.z = *(float*)&buffer.data[buffer_idx + 8];
                        vertices[vertex_idx].weights.w = *(float*)&buffer.data[buffer_idx + 12];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        vertices[vertex_idx].weights.x = *(uint8_t*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].weights.y = *(uint8_t*)&buffer.data[buffer_idx + 1];
                        vertices[vertex_idx].weights.z = *(uint8_t*)&buffer.data[buffer_idx + 2];
                        vertices[vertex_idx].weights.w = *(uint8_t*)&buffer.data[buffer_idx + 3];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        vertices[vertex_idx].weights.x = *(uint16_t*)&buffer.data[buffer_idx + 0];
                        vertices[vertex_idx].weights.y = *(uint16_t*)&buffer.data[buffer_idx + 2];
                        vertices[vertex_idx].weights.z = *(uint16_t*)&buffer.data[buffer_idx + 4];
                        vertices[vertex_idx].weights.w = *(uint16_t*)&buffer.data[buffer_idx + 8];
                        break;
                    default:
                        assert(0);
                    }
                }

                if (attrib.first == std::string("JOINTS_1") || attrib.first == std::string("WEIGHTS_1")) {
                    assert(0); // skinned supports only 4 joint influences for the moment
                }
                vertex_idx++;
            }

        }

        glm::vec3 aabb_half_size = (max_pos - min_pos) * 0.5f;
        glm::vec3 aabb_position = min_pos + aabb_half_size;

        entity_mesh->set_aabb({ aabb_position, aabb_half_size });

        material.flags |= MATERIAL_PBR;

        switch (primitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:
            material.topology_type = TOPOLOGY_TRIANGLE_LIST;
            break;
        case TINYGLTF_MODE_TRIANGLE_STRIP:
            material.topology_type = TOPOLOGY_TRIANGLE_STRIP;
            break;
        case TINYGLTF_MODE_LINE:
            material.topology_type = TOPOLOGY_LINE_LIST;
            break;
        case TINYGLTF_MODE_LINE_STRIP:
            material.topology_type = TOPOLOGY_LINE_STRIP;
            break;
        case TINYGLTF_MODE_POINTS:
            material.topology_type = TOPOLOGY_POINT_LIST;
            break;
        default:
            assert(0);
        }

        if (primitive.material >= 0) {

            tinygltf::Material& gltf_material = model.materials[primitive.material];

            const tinygltf::PbrMetallicRoughness& pbrMetallicRoughness = gltf_material.pbrMetallicRoughness;
            
            if (pbrMetallicRoughness.baseColorTexture.index >= 0) {

                if (texture_cache.contains(pbrMetallicRoughness.baseColorTexture.index)) {
                    material.diffuse_texture = texture_cache[pbrMetallicRoughness.baseColorTexture.index];
                }
                else {
                    create_material_texture(model, pbrMetallicRoughness.baseColorTexture.index, &material.diffuse_texture, true);
                    texture_cache[pbrMetallicRoughness.baseColorTexture.index] = material.diffuse_texture;
                }
            }

            material.color = glm::vec4(
                pbrMetallicRoughness.baseColorFactor[0],
                pbrMetallicRoughness.baseColorFactor[1],
                pbrMetallicRoughness.baseColorFactor[2],
                pbrMetallicRoughness.baseColorFactor[3]
            );

            if (pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
                if (texture_cache.contains(pbrMetallicRoughness.metallicRoughnessTexture.index)) {
                    material.metallic_roughness_texture = texture_cache[pbrMetallicRoughness.metallicRoughnessTexture.index];
                }
                else {
                    create_material_texture(model, pbrMetallicRoughness.metallicRoughnessTexture.index, &material.metallic_roughness_texture);
                    texture_cache[pbrMetallicRoughness.metallicRoughnessTexture.index] = material.metallic_roughness_texture;
                }
            }
           
            material.roughness = static_cast<float>(pbrMetallicRoughness.roughnessFactor);
            material.metalness = static_cast<float>(pbrMetallicRoughness.metallicFactor);

            if (gltf_material.normalTexture.index >= 0) {
                if (texture_cache.contains(gltf_material.normalTexture.index)) {
                    material.normal_texture = texture_cache[gltf_material.normalTexture.index];
                }
                else {
                    create_material_texture(model, gltf_material.normalTexture.index, &material.normal_texture);
                    texture_cache[gltf_material.normalTexture.index] = material.normal_texture;
                }
            }

            if (gltf_material.emissiveTexture.index >= 0) {
                if (texture_cache.contains(gltf_material.emissiveTexture.index)) {
                    material.emissive_texture = texture_cache[gltf_material.emissiveTexture.index];
                }
                else {
                    create_material_texture(model, gltf_material.emissiveTexture.index, &material.emissive_texture, true);
                    texture_cache[gltf_material.emissiveTexture.index] = material.emissive_texture;
                }
            }

            if (gltf_material.occlusionTexture.index >= 0) {
                if (texture_cache.contains(gltf_material.occlusionTexture.index)) {
                    material.oclussion_texture = texture_cache[gltf_material.occlusionTexture.index];
                    material.occlusion = static_cast<float>(gltf_material.occlusionTexture.strength);
                }
                else {
                    create_material_texture(model, gltf_material.occlusionTexture.index, &material.oclussion_texture, true);
                    texture_cache[gltf_material.occlusionTexture.index] = material.oclussion_texture;
                }
            }

            material.emissive = { gltf_material.emissiveFactor[0], gltf_material.emissiveFactor[1], gltf_material.emissiveFactor[2] };

            if (gltf_material.doubleSided) {
                material.cull_type = CULL_NONE;
            } else {
                material.cull_type = CULL_BACK;
            }

            if (gltf_material.alphaMode == "OPAQUE") {
                material.transparency_type = ALPHA_OPAQUE;
            } else
            if (gltf_material.alphaMode == "BLEND") {
                material.transparency_type = ALPHA_BLEND;
            } else
            if (gltf_material.alphaMode == "MASK") {
                material.alpha_mask = static_cast<float>(gltf_material.alphaCutoff);
                material.transparency_type = ALPHA_MASK;
            }
        }
        else {
            // create default material
            material.color = glm::vec4(1.0f);
            material.roughness = 1.0f;
            material.metalness = 0.0f;
        }

        material.shader = RendererStorage::get_shader("data/shaders/mesh_pbr.wgsl", material);

        if (entity_mesh->is_skinned) {
            material.use_skinning = true;
            material.shader = RendererStorage::get_shader("data/shaders/mesh_pbr_skinning.wgsl", material);
            //TO DO : something I dont remember

        }
        surface->create_vertex_buffer();
        surface->set_material_priority(1);
        entity_mesh->add_surface(surface);
    }
}

Node3D* create_node_entity(tinygltf::Node& node) {

    Node3D* new_node = nullptr;

    if (node.mesh >= 0) {
        new_node = new MeshInstance3D();
        if (node.skin >= 0) {
            ((MeshInstance3D*)(new_node))->is_skinned = true;

        }
    }
    else if (node.camera >= 0) {
        new_node = new EntityCamera();
    }
    else {
        new_node = new Node3D();
    }

    return new_node;
};

void parse_model_nodes(tinygltf::Model& model, int id, Node3D* entity, std::map<uint32_t, Texture*> &texture_cache, std::map<int, int> &hierarchy, std::vector<SkeletonInstance3D*> & skeleton_instances ) {

    tinygltf::Node node = model.nodes[id];

    entity->set_name(node.name);

    if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
        read_mesh(model, model.meshes[node.mesh], entity, texture_cache);

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
            rotation_quat = glm::quat(static_cast<float>(node.rotation[0]),
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

        entity->set_model( glm::toMat4(rotation_quat) * scale_matrix * translation_matrix);
    }


    // Parse children
    for (size_t i = 0; i < node.children.size(); i++) {
        assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
        bool is_joint = false;
        Node3D* child_node = create_node_entity(model.nodes[node.children[i]]);

        if (model.nodes[node.children[i]].skin >= 0) {
            SkeletonInstance3D* e = new SkeletonInstance3D();
            //e->set_model(inverse(entity->get_model())); // Harcoded 
            e->set_name(entity->get_name() + "_" + "skeleton");
            e->skin = model.nodes[node.children[i]].skin;
            entity->add_child(e);
            e->add_child(child_node);

            skeleton_instances.push_back(e);       

        }
        else {

            for (size_t s = 0; s < model.skins.size(); s++) {
                auto it = std::find(model.skins[s].joints.begin(), model.skins[s].joints.end(), node.children[i]);
                if (it != model.skins[s].joints.end()) {
                    is_joint = true;
                    hierarchy[node.children[i]] = id;
                    break;
                }
            }
            if (!is_joint) {
                entity->add_child(child_node);
            }
        }
        parse_model_nodes(model, node.children[i], child_node, texture_cache, hierarchy, skeleton_instances);

    }
};

void parse_model_skins(tinygltf::Model& model, std::map<int, int> hierarchy, std::vector<SkeletonInstance3D*>& skeleton_instances) {
    if (!model.skins.size() ) {
        return;
    }

    std::vector<Transform> world_bind_transforms;

    for (size_t s = 0; s < model.skins.size(); s++) {
        const tinygltf::Skin& skin = model.skins[s];
        if (skin.inverseBindMatrices > -1) {

            Pose bind_pose;
            Pose rest_pose;

            size_t num_joints = skin.joints.size();
            if (num_joints > 0) {

                rest_pose.resize(num_joints);
                world_bind_transforms.resize(num_joints);

                // read the inverse bind pose matrix into a large vector of float values
                const tinygltf::Accessor& accessor = model.accessors[skin.inverseBindMatrices];
                assert(accessor.type == TINYGLTF_TYPE_MAT4);

                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const float* ptr = reinterpret_cast<const float*>(buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);
                

                std::vector<glm::mat4> inverse_bind_matrices(accessor.count);
                //for each joint in the skin, get the inverse bind pose matrix

                for (size_t i = 0; i < skin.joints.size(); i++) {

                    tinygltf::Node node = model.nodes[skin.joints[i]];

                    if (!node.matrix.empty())
                    {
                        glm::mat4x4 model_matrix;

                        for (int j = 0; j < 16; ++j) {
                            model_matrix[(j / 4) % 4][j % 4] = static_cast<float>(node.matrix[j]);
                        }

                        rest_pose.set_local_transform(skin.joints[i], mat4ToTransform(model_matrix));
                        
                    }
                    else {
                        Transform transform;               

                        if (!node.translation.empty()) {

                            transform.position = glm::vec3({
                                static_cast<float>(node.translation[0]),
                                static_cast<float>(node.translation[1]),
                                static_cast<float>(node.translation[2])
                                });
                        }
                        if (!node.rotation.empty()) {
                            transform.rotation = glm::quat(static_cast<float>(node.rotation[0]),
                                static_cast<float>(node.rotation[1]),
                                static_cast<float>(node.rotation[2]),
                                static_cast<float>(node.rotation[3])
                            );
                        }
                        if (!node.scale.empty()) {
                            transform.scale = glm::vec3({
                                static_cast<float>(node.scale[0]),
                                static_cast<float>(node.scale[1]),
                                static_cast<float>(node.scale[2])
                                });
                        }

                       
                        rest_pose.set_local_transform(skin.joints[i], transform);
                    }

                    rest_pose.set_parent(skin.joints[i], hierarchy[skin.joints[i]]);

                    glm::highp_f32mat4 m;
                    //get the 16 values of the inverse bind matrix and put them into a mat4
                    memcpy(&m, ptr + i * 16, 16 * sizeof(float));
                    m = m;
                    inverse_bind_matrices[skin.joints[i]] = m;
                    //set the transform into the array of transforms of the joints in the bind pose (world bind pose)
                    glm::mat4x4 bind_matrix = inverse(m);
                    Transform bind_transform = mat4ToTransform(bind_matrix);
                    world_bind_transforms[skin.joints[i]] = bind_transform;
                
                }
                bind_pose = rest_pose;

                for (unsigned int i = 0; i < num_joints; ++i) {
                    Transform current = world_bind_transforms[i];
                    int p = bind_pose.get_parent(i);
                    if (p >= 0 ) { // Bring into local space
                        Transform parent;
                        if (p < num_joints) {
                            parent = world_bind_transforms[p];
                        }
                        else {
                            tinygltf::Node node = model.nodes[p];
                            if (!node.matrix.empty())
                            {
                                glm::mat4x4 model_matrix;

                                for (int j = 0; j < 16; ++j) {
                                    model_matrix[(j / 4) % 4][j % 4] = static_cast<float>(node.matrix[j]);
                                }
                            }
                            else {
                                Transform transform;

                                if (!node.translation.empty()) {

                                    transform.position = glm::vec3({
                                        static_cast<float>(node.translation[0]),
                                        static_cast<float>(node.translation[1]),
                                        static_cast<float>(node.translation[2])
                                        });
                                }
                                if (!node.rotation.empty()) {
                                    transform.rotation = glm::quat(static_cast<float>(node.rotation[0]),
                                        static_cast<float>(node.rotation[1]),
                                        static_cast<float>(node.rotation[2]),
                                        static_cast<float>(node.rotation[3])
                                    );
                                }
                                if (!node.scale.empty()) {
                                    transform.scale = glm::vec3({
                                        static_cast<float>(node.scale[0]),
                                        static_cast<float>(node.scale[1]),
                                        static_cast<float>(node.scale[2])
                                        });
                                }
                                parent = transform;
                            }
                        }
                        current = combine(inverse(parent), current);
                    }
                    bind_pose.set_local_transform(i, current);
                }

                std::vector<std::string> joint_names;

                Skeleton *skeleton = new Skeleton(rest_pose, bind_pose, joint_names);
                for (auto instance : skeleton_instances) {

                    if (instance->skin == s) {

                        instance->set_skeleton(skeleton);

                        for (auto child : instance->get_children()) {
                            MeshInstance* child_instance = dynamic_cast<MeshInstance*>(child);
                            assert(child_instance);
                            child_instance->set_skeleton(skeleton);
                        }
                    }
                }
                //skin_map[s]->set_skeleton(skeleton);
                //skinning_matrices[s].inverseBindMatrices = inverse_bind_matrices;
            }
        }
    }

};

bool parse_gltf(const char* gltf_path, std::vector<Node3D*>& entities)
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


    const tinygltf::Scene* scene = nullptr;

    if (model.defaultScene >= 0) {
        scene = &model.scenes[model.defaultScene];
    }
    else {
        scene = &model.scenes[0];
    }

    std::map<uint32_t, Texture*> texture_cache;

    int entity_name_idx = 0;
    std::map<int, int> hierarchy;
    std::vector<SkeletonInstance3D*> skeleton_instances;

    for (size_t i = 0; i < scene->nodes.size(); ++i) {
        assert((scene->nodes[i] >= 0) && (scene->nodes[i] < model.nodes.size()));

        tinygltf::Node node = model.nodes[scene->nodes[i]];

        Node3D* entity = create_node_entity(node);

        parse_model_nodes(model, scene->nodes[i], entity, texture_cache, hierarchy, skeleton_instances);

       

        if (entity->get_name().empty()) {
            entity->set_name(path.stem().string() + "_" + std::to_string(entity_name_idx));
            entity_name_idx++;
        }

        entities.push_back(entity);
    }

    parse_model_skins(model, hierarchy, skeleton_instances);

    texture_cache.clear();

    return true;
}
