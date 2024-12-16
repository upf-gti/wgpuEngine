#include "parse_gltf.h"

#include "glm/glm.hpp"

#include "json.hpp"

#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "framework/nodes/camera.h"
#include "framework/nodes/mesh_instance_3d.h"
#include "framework/nodes/skeleton_instance_3d.h"
#include "framework/nodes/joint_3d.h"
#include "framework/nodes/animation_player.h"
#include "framework/nodes/directional_light_3d.h"
#include "framework/nodes/spot_light_3d.h"
#include "framework/nodes/omni_light_3d.h"
#include "framework/nodes/look_at_ik_3d.h"

#include "graphics/texture.h"
#include "graphics/shader.h"
#include "graphics/renderer_storage.h"

#include "engine/scene.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "spdlog/spdlog.h"

void create_material_texture(const tinygltf::Model& model, int tex_index, Texture** texture, bool is_srgb = false, bool fill_texture_data = false)
{
    const tinygltf::Texture& tex = model.textures[tex_index];

    if (tex.source < 0)
        return;

    static uint32_t texture_idx = 0;

    const tinygltf::Image& image = model.images[tex.source];

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

        if (Texture::convert_to_rgba8unorm(image.width, image.height, texture_format, (void*)image.image.data(), converted_texture)) {
            *texture = new Texture();
            (*texture)->load_from_data(image.uri, WGPUTextureDimension_2D, image.width, image.height, 1, converted_texture, true, is_srgb ? WGPUTextureFormat_RGBA8UnormSrgb : WGPUTextureFormat_RGBA8Unorm);


            if (fill_texture_data) {
                std::vector<uint8_t>& texture_data = (*texture)->get_texture_data();
                texture_data.assign(converted_texture, converted_texture + image.width * image.height * 4);
            }

            delete[] converted_texture;
        }
    }
    else {
        *texture = new Texture();
        (*texture)->load_from_data(image.uri, WGPUTextureDimension_2D, image.width, image.height, 1, (void*)image.image.data(), true, is_srgb ? WGPUTextureFormat_RGBA8UnormSrgb : WGPUTextureFormat_RGBA8Unorm);

        if (fill_texture_data) {
            std::vector<uint8_t>& texture_data = (*texture)->get_texture_data();
            texture_data.assign(image.image.data(), image.image.data() + image.width * image.height * 4);
        }
    }

    if (image.name.empty()) {
        (*texture)->set_name("texture_" + std::to_string(texture_idx));
        texture_idx++;
    }
    else {
        (*texture)->set_name(image.name);
    }

    if (tex.sampler != -1)
    {
        const tinygltf::Sampler& sampler = model.samplers[tex.sampler];

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

void read_transform(const tinygltf::Node& node, Transform& transform)
{
    if (!node.translation.empty()) {
        transform.set_position({ static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2]) });
    }
    if (!node.rotation.empty()) {
        transform.set_rotation({ static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]), static_cast<float>(node.rotation[3]) });
    }
    if (!node.scale.empty()) {
        transform.set_scale({ static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]) });
        /* transform.scale.x = transform.scale.x >= 0.99999f ? 1.f : transform.scale.x;
         transform.scale.y = transform.scale.y >= 0.99999f ? 1.f : transform.scale.y;
         transform.scale.z = transform.scale.z >= 0.99999f ? 1.f : transform.scale.z;*/

    }
}

void parse_attribute(tinygltf::Buffer const& buffer, size_t buffer_start, size_t buffer_end, size_t buffer_size, size_t stride, uint8_t* attribute_ptr, uint16_t attribute_byte_size, std::function<void(tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr)> custom_parse = nullptr)
{
    size_t vertex_idx = 0;

    for (size_t buffer_idx = buffer_start; buffer_idx < buffer_end; buffer_idx += stride) {
        if (vertex_idx >= buffer_size) {
            assert(0);
            break;
        }

        if (custom_parse) {
            custom_parse(buffer, buffer_idx, &attribute_ptr);
        }
        else {
            memcpy(attribute_ptr, &buffer.data[buffer_idx], attribute_byte_size);
        }

        attribute_ptr += attribute_byte_size;

        vertex_idx++;
    }
}

void read_mesh(const tinygltf::Model& model, const tinygltf::Node& node, Node3D* entity, std::map<uint32_t, Texture*>& texture_cache, std::map<size_t, Surface*>& mesh_cache, bool fill_surface_data)
{
    const tinygltf::Mesh& mesh = model.meshes[node.mesh];
    uint32_t joints_count = 0;
    for (int i = 0; i < node.skin; i++) {
        joints_count += static_cast<uint32_t>(model.skins[i].joints.size());
    }

    MeshInstance3D* entity_mesh = dynamic_cast<MeshInstance3D*>(entity);

    if (!entity_mesh) {
        assert(0);
        return;
    }

    glm::vec3 min_pos = { FLT_MAX, FLT_MAX, FLT_MAX };
    glm::vec3 max_pos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (size_t primitive_idx = 0; primitive_idx < mesh.primitives.size(); ++primitive_idx) {

        const tinygltf::Primitive& primitive = mesh.primitives[primitive_idx];

        GltfPrimitive gltf_primitive = { primitive.attributes, primitive.material, primitive.indices, primitive.mode };

        size_t hash = std::hash<GltfPrimitive>()(gltf_primitive);

        if (mesh_cache.contains(hash)) {
            Surface* surface = mesh_cache[hash];
            entity_mesh->add_surface(surface);
            continue;
        }

        Surface* surface = new Surface();
        mesh_cache[hash] = surface;

        Material* material = new Material();

        sSurfaceData vertices;

        size_t index_data_size = 0;

        bool uses_indices = primitive.indices >= 0;

        if (uses_indices) {
            tinygltf::BufferView const* indices_buffer_view = nullptr;
            tinygltf::Accessor const* index_accessor = nullptr;
            tinygltf::Buffer const* indices_buffer = nullptr;

            index_accessor = &model.accessors[primitive.indices];

            indices_buffer_view = &model.bufferViews[index_accessor->bufferView];
            indices_buffer = &model.buffers[indices_buffer_view->buffer];
            size_t index_buffer_size = 0;

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
                index_data_size = index_coponent_size * sizeof(uint32_t);
                index_buffer_size = indices_buffer_view->byteLength / index_data_size;
                break;
            }

            size_t index_stride;

            if (indices_buffer_view->byteStride == 0) {
                index_stride = index_data_size;
            }
            else {
                index_stride = indices_buffer_view->byteStride;
            }

            size_t index_buffer_start = index_accessor->byteOffset + indices_buffer_view->byteOffset;
            size_t index_buffer_end = index_stride * index_accessor->count + index_buffer_start;

            uint32_t index_idx = 0;

            vertices.indices.resize((index_buffer_end - index_buffer_start) / index_stride);

            for (size_t buffer_idx = index_buffer_start; buffer_idx < index_buffer_end; buffer_idx += index_stride) {
                if (index_idx >= vertices.indices.size())
                    break;

                uint32_t index = 0;
                switch (index_accessor->componentType) {
                    //case TINYGLTF_COMPONENT_TYPE_BYTE:
                    //    index = *(int8_t*)&(indices_buffer->data[index_idx]);
                    //    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    index = *(uint8_t*)&(indices_buffer->data[buffer_idx]);
                    break;
                    //case TINYGLTF_COMPONENT_TYPE_SHORT:
                    //    index = *(int16_t*)&(indices_buffer->data[index_idx]);
                    //    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index = *(uint16_t*)&(indices_buffer->data[buffer_idx]);
                    break;
                    //case TINYGLTF_COMPONENT_TYPE_INT:
                    //    index = *(int32_t*)&(indices_buffer->data[index_idx]);
                    //    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index = *(uint32_t*)&(indices_buffer->data[buffer_idx]);
                    break;
                default:
                    assert(0);
                }

                vertices.indices[index_idx] = index;

                index_idx++;
            }

            surface->create_index_buffer(vertices.indices);
        }

        for (auto& attrib : primitive.attributes) {

            const tinygltf::Accessor& accessor = model.accessors[attrib.second];
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

            size_t stride;

            if (buffer_view.byteStride == 0) {
                stride = vertex_attribute_size;
            }
            else {
                stride = buffer_view.byteStride;
            }

            size_t buffer_start = accessor.byteOffset + buffer_view.byteOffset;
            size_t buffer_end = stride * accessor.count + buffer_start;
            size_t buffer_size = (buffer_end - buffer_start) / stride;

            // position
            if (attrib.first[0] == 'P') {
                vertices.vertices.resize(buffer_size);

                parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.vertices[0]), sizeof(float) * 3,
                    [&](tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr) {

                        memcpy(*attribute_ptr, &buffer.data[buffer_idx], sizeof(float) * 3);

                        glm::vec3* position = reinterpret_cast<glm::vec3*>(*attribute_ptr);

                        // For AABB
                        min_pos = glm::min(*position, min_pos);
                        max_pos = glm::max(*position, max_pos);
                    }
                );
            }

            // normal
            if (attrib.first[0] == 'N') {
                vertices.normals.resize(buffer_size);

                parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.normals[0]), sizeof(float) * 3);
            }

            // uv
            if (attrib.first == std::string("TEXCOORD_0")) {
                vertices.uvs.resize(buffer_size);

                parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.uvs[0]), sizeof(float) * 2);
            }

            // tangents
            if (attrib.first[0] == 'T' && attrib.first[1] == 'A') {
                vertices.tangents.resize(buffer_size);

                parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.tangents[0]), sizeof(float) * 4);
            }

            // color
            if (attrib.first == std::string("COLOR_0")) {
                vertices.colors.resize(buffer_size);

                switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.colors[0]), sizeof(float) * 3);
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.colors[0]), sizeof(float) * 3,
                        [](tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr) {
                            glm::u8vec3 color_u8;
                            memcpy(&color_u8[0], &buffer.data[buffer_idx], sizeof(uint8_t) * 3);

                            glm::vec3* color = reinterpret_cast<glm::vec3*>(*attribute_ptr);
                            color->x = color_u8.x / 255.0f;
                            color->y = color_u8.y / 255.0f;
                            color->z = color_u8.z / 255.0f;
                        }
                    );
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.colors[0]), sizeof(float) * 3,
                        [](tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr) {
                            glm::u16vec3 color_u16;
                            memcpy(&color_u16[0], &buffer.data[buffer_idx], sizeof(uint16_t) * 3);

                            glm::vec3* color = reinterpret_cast<glm::vec3*>(*attribute_ptr);
                            color->x = color_u16.x / 65535.0f;
                            color->y = color_u16.y / 65535.0f;
                            color->z = color_u16.z / 65535.0f;
                        }
                    );
                    break;
                default:
                    assert(0);
                }
            }

            // joints (skinning)
            if (attrib.first == std::string("JOINTS_0")) {
                vertices.joints.resize(buffer_size);

                switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.joints[0]), sizeof(uint32_t) * 4,
                        [=](tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr) {

                            glm::u8vec4 joints_u8;
                            memcpy(&joints_u8[0], &buffer.data[buffer_idx], sizeof(uint8_t) * 4);

                            glm::ivec4* joints = reinterpret_cast<glm::ivec4*>(*attribute_ptr);
                            *joints = joints_u8;
                            *joints += static_cast<uint32_t>(joints_count);
                        }
                    );
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.joints[0]), sizeof(uint32_t) * 4,
                        [=](tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr) {

                            glm::u16vec4 joints_u16;
                            memcpy(&joints_u16[0], &buffer.data[buffer_idx], sizeof(uint16_t) * 4);

                            glm::ivec4* joints = reinterpret_cast<glm::ivec4*>(*attribute_ptr);
                            *joints = joints_u16;
                            *joints += static_cast<int32_t>(joints_count);
                        }
                    );
                    break;
                default:
                    assert(0);
                }
            }

            // weights (skinning)
            if (attrib.first == std::string("WEIGHTS_0")) {
                vertices.weights.resize(buffer_size);

                switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.weights[0]), sizeof(float) * 4);
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.weights[0]), sizeof(float) * 4,
                        [](tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr) {
                            glm::u8vec4 weights_u8;
                            memcpy(&weights_u8[0], &buffer.data[buffer_idx], sizeof(uint8_t) * 4);

                            glm::vec4* weights = reinterpret_cast<glm::vec4*>(*attribute_ptr);
                            *weights = weights_u8;

                            //Make sure that even the invalid nodes have a value of 0 (any negative joint indices will break the skinning implementation)
                            float sum = weights->x + weights->y + weights->z + weights->w;
                            *weights = glm::clamp(*weights, 0.0f, 1.0f);
                            *weights /= sum;
                            //weights.w = 0;
                        }
                    );
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    parse_attribute(buffer, buffer_start, buffer_end, buffer_size, stride, reinterpret_cast<uint8_t*>(&vertices.weights[0]), sizeof(float) * 4,
                        [](tinygltf::Buffer const& buffer, size_t buffer_idx, uint8_t** attribute_ptr) {
                            glm::u16vec4 weights_u16;
                            memcpy(&weights_u16[0], &buffer.data[buffer_idx], sizeof(uint16_t) * 4);

                            glm::vec4* weights = reinterpret_cast<glm::vec4*>(*attribute_ptr);
                            *weights = weights_u16;

                            //Make sure that even the invalid nodes have a value of 0 (any negative joint indices will break the skinning implementation)
                            float sum = weights->x + weights->y + weights->z + weights->w;
                            *weights = glm::clamp(*weights, 0.0f, 1.0f);
                            *weights /= sum;
                            //weights.w = 0;
                        }
                    );
                    break;
                default:
                    assert(0);
                }
            }

            if (attrib.first == std::string("JOINTS_1") || attrib.first == std::string("WEIGHTS_1")) {
                assert(0); // skinned supports only 4 joint influences for the moment
            }
        }

        glm::vec3 aabb_half_size = (max_pos - min_pos) * 0.5f;
        glm::vec3 aabb_position = min_pos + aabb_half_size;

        surface->set_aabb({ aabb_position, aabb_half_size });

        switch (primitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:
            material->set_topology_type(TOPOLOGY_TRIANGLE_LIST);
            break;
        case TINYGLTF_MODE_TRIANGLE_STRIP:
            material->set_topology_type(TOPOLOGY_TRIANGLE_STRIP);
            break;
        case TINYGLTF_MODE_LINE:
            material->set_topology_type(TOPOLOGY_LINE_LIST);
            break;
        case TINYGLTF_MODE_LINE_STRIP:
            material->set_topology_type(TOPOLOGY_LINE_STRIP);
            break;
        case TINYGLTF_MODE_POINTS:
            material->set_topology_type(TOPOLOGY_POINT_LIST);
            break;
        default:
            assert(0);
        }

        bool tangents_generated = false;
        if (primitive.mode == TINYGLTF_MODE_TRIANGLES && !(vertices.normals.empty()) && vertices.tangents.empty()) {
            tangents_generated = surface->generate_tangents(&vertices);
        }

        if (primitive.material >= 0) {

            const tinygltf::Material& gltf_material = model.materials[primitive.material];

            const tinygltf::PbrMetallicRoughness& pbrMetallicRoughness = gltf_material.pbrMetallicRoughness;

            for (const auto& extension : gltf_material.extensions) {
                if (extension.first == "KHR_materials_unlit") {
                    material->set_type(MATERIAL_UNLIT);
                }
            }

            if (pbrMetallicRoughness.baseColorTexture.index >= 0) {

                if (texture_cache.contains(pbrMetallicRoughness.baseColorTexture.index)) {
                    material->set_diffuse_texture(texture_cache[pbrMetallicRoughness.baseColorTexture.index]);
                }
                else {
                    Texture* diffuse_texture;
                    create_material_texture(model, pbrMetallicRoughness.baseColorTexture.index, &diffuse_texture, true, fill_surface_data);
                    texture_cache[pbrMetallicRoughness.baseColorTexture.index] = diffuse_texture;
                    material->set_diffuse_texture(diffuse_texture);
                }
            }
            material->set_name(gltf_material.name);
            material->set_color(glm::vec4(
                pbrMetallicRoughness.baseColorFactor[0],
                pbrMetallicRoughness.baseColorFactor[1],
                pbrMetallicRoughness.baseColorFactor[2],
                pbrMetallicRoughness.baseColorFactor[3]
            ));

            if (pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
                if (texture_cache.contains(pbrMetallicRoughness.metallicRoughnessTexture.index)) {
                    material->set_metallic_roughness_texture(texture_cache[pbrMetallicRoughness.metallicRoughnessTexture.index]);
                }
                else {
                    Texture* metallic_roughness_texture;
                    create_material_texture(model, pbrMetallicRoughness.metallicRoughnessTexture.index, &metallic_roughness_texture);
                    texture_cache[pbrMetallicRoughness.metallicRoughnessTexture.index] = metallic_roughness_texture;
                    material->set_metallic_roughness_texture(metallic_roughness_texture);
                }
            }

            material->set_roughness(static_cast<float>(pbrMetallicRoughness.roughnessFactor));
            material->set_metallic(static_cast<float>(pbrMetallicRoughness.metallicFactor));

            if (gltf_material.normalTexture.index >= 0) {
                if (texture_cache.contains(gltf_material.normalTexture.index)) {
                    material->set_normal_texture(texture_cache[gltf_material.normalTexture.index]);
                }
                else {
                    Texture* normal_texture;
                    create_material_texture(model, gltf_material.normalTexture.index, &normal_texture);
                    texture_cache[gltf_material.normalTexture.index] = normal_texture;
                    material->set_normal_texture(normal_texture);
                }
            }

            if (gltf_material.emissiveTexture.index >= 0) {
                if (texture_cache.contains(gltf_material.emissiveTexture.index)) {
                    material->set_emissive_texture(texture_cache[gltf_material.emissiveTexture.index]);
                }
                else {
                    Texture* emissive_texture;
                    create_material_texture(model, gltf_material.emissiveTexture.index, &emissive_texture, true);
                    texture_cache[gltf_material.emissiveTexture.index] = emissive_texture;
                    material->set_emissive_texture(emissive_texture);
                }
            }

            if (gltf_material.occlusionTexture.index >= 0) {
                if (texture_cache.contains(gltf_material.occlusionTexture.index)) {
                    material->set_occlusion_texture(texture_cache[gltf_material.occlusionTexture.index]);
                    material->set_occlusion(static_cast<float>(gltf_material.occlusionTexture.strength));
                }
                else {
                    Texture* occlusion_texture;
                    create_material_texture(model, gltf_material.occlusionTexture.index, &occlusion_texture, true);
                    texture_cache[gltf_material.occlusionTexture.index] = occlusion_texture;
                    material->set_occlusion_texture(occlusion_texture);
                }
            }

            material->set_emissive({ gltf_material.emissiveFactor[0], gltf_material.emissiveFactor[1], gltf_material.emissiveFactor[2] });

            if (gltf_material.doubleSided) {
                material->set_cull_type(CULL_NONE);
            }
            else {
                material->set_cull_type(CULL_BACK);
            }

            if (gltf_material.alphaMode == "OPAQUE") {
                material->set_transparency_type(ALPHA_OPAQUE);
            }
            else if (gltf_material.alphaMode == "BLEND") {
                material->set_transparency_type(ALPHA_BLEND);
            }
            else if (gltf_material.alphaMode == "MASK") {
                material->set_alpha_mask(static_cast<float>(gltf_material.alphaCutoff));
                material->set_transparency_type(ALPHA_MASK);
            }
        }
        else {
            // create default material
            material->set_color(glm::vec4(1.0f));
            material->set_roughness(1.0f);
            material->set_metallic(0.0f);
        }

        if (entity_mesh->is_skinned) {
            material->set_use_skinning(true);
        }

        material->set_priority(1);

        std::vector<std::string> custom_defines;

        if (tangents_generated || primitive.attributes.contains("TANGENT")) {
            custom_defines.push_back("HAS_TANGENTS");
        }

        material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries, material, custom_defines));

        surface->set_material(material);

        surface->create_surface_data(vertices, fill_surface_data);
        surface->set_name("Surface_" + material->get_name());
        entity_mesh->add_surface(surface);
    }

    AABB entity_aabb;
    for (const Surface* surface : entity_mesh->get_surfaces()) {
        const AABB& surface_aabb = surface->get_aabb().transform(entity_mesh->get_global_model());
        entity_aabb = merge_aabbs(entity_aabb, surface_aabb);
    }

    entity_mesh->set_aabb(entity_aabb);
}

void create_camera(const tinygltf::Camera& gltf_camera, EntityCamera* camera_node)
{
    if (gltf_camera.type == "perspective") {
        camera_node->set_perspective(
            static_cast<float>(gltf_camera.perspective.yfov),
            static_cast<float>(gltf_camera.perspective.aspectRatio),
            static_cast<float>(gltf_camera.perspective.znear),
            static_cast<float>(gltf_camera.perspective.zfar)
        );
    }
    else {
        camera_node->set_orthographic(
            0.0f,
            static_cast<float>(gltf_camera.orthographic.xmag),
            static_cast<float>(gltf_camera.orthographic.ymag),
            0.0f,
            static_cast<float>(gltf_camera.orthographic.znear),
            static_cast<float>(gltf_camera.orthographic.zfar)
        );
    }
}

void create_light(const tinygltf::Light& gltf_light, Node3D** light_node)
{
    std::string light_type = gltf_light.type;

    if (light_type == "spot") {
        SpotLight3D* new_spot = new SpotLight3D();
        new_spot->set_inner_cone_angle(static_cast<float>(gltf_light.spot.innerConeAngle));
        new_spot->set_outer_cone_angle(static_cast<float>(gltf_light.spot.outerConeAngle));
        *light_node = new_spot;
    }
    else if (light_type == "point") {
        OmniLight3D* new_omni = new OmniLight3D();
        *light_node = new_omni;
    }
    else if (light_type == "directional") {
        DirectionalLight3D* new_directional = new DirectionalLight3D();
        *light_node = new_directional;
    }
    else {
        assert(0);
        return;
    }

    Light3D* light = static_cast<Light3D*>(*light_node);

    float light_intensity = static_cast<float>(gltf_light.intensity);

    // TODO: fix this hack if we ever implement physical light units
    // this makes blender lights look ok'ish
    if (light_intensity > 100.0) {
        light_intensity /= 100.0f;
    }

    light->set_intensity(light_intensity);

    // Blender exports 0.0 if no custom distance is used.. so keep -1 as range
    if (gltf_light.range != 0.0f) {
        light->set_range(static_cast<float>(gltf_light.range));
    }
    if (gltf_light.color.size()) {
        light->set_color({ static_cast<float>(gltf_light.color[0]), static_cast<float>(gltf_light.color[1]), static_cast<float>(gltf_light.color[2]) });
    }
}

void register_node(std::string& name, Node3D* node, std::map<std::string, Node3D*>& loaded_nodes, std::map<std::string, uint32_t>& name_repeats)
{
    if (name_repeats.contains(name)) {
        uint32_t& count = name_repeats[name];
        std::string count_str = std::to_string(count);
        uint32_t num_zeros = std::min(2, 3 - (int)count_str.size());
        name += "_" + count_str.insert(0, num_zeros, '0');
        count++;
    }
    else {
        name_repeats[name] = 1;
    }

    loaded_nodes[name] = node;
}

Node3D* create_node_entity(uint32_t node_id, tinygltf::Model& model, std::map<std::string, Node3D*>& loaded_nodes, std::map<std::string, uint32_t>& name_repeats)
{
    tinygltf::Node& node = model.nodes[node_id];

    Node3D* new_node = nullptr;

    if (node.mesh >= 0) {
        new_node = new MeshInstance3D();
        if (node.skin >= 0) {
            static_cast<MeshInstance3D*>(new_node)->is_skinned = true;
        }
    }
    else if (node.camera >= 0) {
        new_node = new EntityCamera();

        const tinygltf::Camera& gltf_camera = model.cameras[node.camera];

        create_camera(gltf_camera, static_cast<EntityCamera*>(new_node));
    }
    else if (node.light >= 0) {
        const tinygltf::Light& gltf_light = model.lights[node.light];
        create_light(gltf_light, &new_node);
    }
    else {
        new_node = new Node3D();
    }

    if (node.name == "") {

        if (node.mesh >= 0) {

            const tinygltf::Mesh& mesh = model.meshes[node.mesh];

            if (mesh.name != "") {
                node.name = mesh.name;
            }
        }

        if (node.name == "") {
            node.name = "Node_" + std::to_string(node_id);
        }
    }

    // Register to get access by gltf node name later..
    register_node(node.name, new_node, loaded_nodes, name_repeats);

    new_node->set_name(node.name);

    return new_node;
};

void process_node_hierarchy(const tinygltf::Model& model, int parent_id, uint32_t node_id, Node3D* parent_node, Node3D* entity, std::map<int, int>& hierarchy, std::vector<SkeletonInstance3D*>& skeleton_instances)
{
    bool is_joint = false;

    for (size_t s = 0; s < model.skins.size(); s++) {
        auto it = std::find(model.skins[s].joints.begin(), model.skins[s].joints.end(), node_id);
        if (it != model.skins[s].joints.end()) {
            is_joint = true;
            break;
        }
    }

    if (is_joint) {
        entity->set_node_type("Joint3D");
    }
    else {
        parent_node->add_child(entity);
    }

    hierarchy[node_id] = parent_id;
}

void parse_model_nodes(tinygltf::Model& model, int parent_id, uint32_t node_id, Node3D* parent_node, Node3D* entity, std::map<std::string, Node3D*>& loaded_nodes, std::map<std::string, uint32_t>& name_repeats,
    std::map<uint32_t, Texture*>& texture_cache, std::map<size_t, Surface*>& mesh_cache, std::map<int, int>& hierarchy, std::vector<SkeletonInstance3D*>& skeleton_instances, bool fill_surface_data)
{
    tinygltf::Node& node = model.nodes[node_id];

    // Set model matrix
    if (!node.matrix.empty())
    {
        glm::mat4x4 model_matrix;

        for (int j = 0; j < 16; ++j) {
            model_matrix[(j / 4) % 4][j % 4] = static_cast<float>(node.matrix[j]);
        }

        entity->set_transform(Transform::mat4_to_transform(model_matrix));
    }
    else {

        Transform transform;
        read_transform(node, transform);

        entity->set_transform(transform);
    }

    if (node.mesh >= 0 && node.mesh < model.meshes.size()) {
        read_mesh(model, node, entity, texture_cache, mesh_cache, fill_surface_data);
        AABB parent_aabb = merge_aabbs(entity->get_aabb(), parent_node->get_aabb());
        parent_node->set_aabb(parent_aabb);
    }

    // Parse children
    for (size_t i = 0; i < node.children.size(); i++) {

        uint32_t child_id = node.children[i];

        assert(child_id >= 0 && child_id < model.nodes.size());

        Node3D* child_node = create_node_entity(child_id, model, loaded_nodes, name_repeats);

        process_node_hierarchy(model, node_id, child_id, entity, child_node, hierarchy, skeleton_instances);

        parse_model_nodes(model, node_id, child_id, entity, child_node, loaded_nodes, name_repeats, texture_cache, mesh_cache, hierarchy, skeleton_instances, fill_surface_data);
    }
};

void parse_model_skins(Node3D* scene_root, tinygltf::Model& model, std::map<std::string, Node3D*>& loaded_nodes, std::map<int, int>& hierarchy, std::vector<SkeletonInstance3D*>& skeleton_instances)
{
    // Create skeleton instances first..
    {
        for (size_t i = 0; i < model.nodes.size(); i++) {

            const tinygltf::Node& node = model.nodes[i];

            if (node.skin < 0) {
                continue;
            }

            // Insert at joint root parent..

            tinygltf::Skin& skin = model.skins[node.skin];
            assert(skin.joints.size() > 0);

            if (skin.name == "") {
                skin.name = "Skin_" + std::to_string(node.skin);
            }

            // Assuming the first join in the array will be the root joint,
            // get the parent node from the hierarchy

            int joint_root_id = skin.joints[0];
            int joint_root_parent_id = hierarchy[joint_root_id];

            Node3D* parent_node = scene_root;

            assert(loaded_nodes.contains(node.name));

            Node3D* skinned_mesh_node = loaded_nodes[node.name];
            SkeletonInstance3D* e = nullptr;

            if (joint_root_parent_id >= 0) {

                const tinygltf::Node& parent = model.nodes[joint_root_parent_id];
                assert(loaded_nodes.contains(parent.name));
                parent_node = loaded_nodes[parent.name];
                if (parent_node->get_node_type() != "Joint3D") {
                    e = new SkeletonInstance3D();
                    parent_node->add_child(e);
                    skeleton_instances.push_back(e);
                }
            }
            if (!skeleton_instances.size()) {
                e = new SkeletonInstance3D();
                parent_node->add_child(e);
                skeleton_instances.push_back(e);
            }
            else {

            }

            e = skeleton_instances[skeleton_instances.size() - 1];
            e->add_child(skinned_mesh_node);
        }
    }

    std::vector<Transform> world_bind_transforms;
    Pose bind_pose;
    Pose rest_pose;

    std::vector<glm::mat4> inverse_bind_matrices;
    std::vector<std::string> joint_names;
    std::vector<uint32_t> joint_indices;
    std::vector<Joint3D*> joint_nodes;

    for (size_t s = 0; s < model.skins.size(); s++) {

        const tinygltf::Skin& skin = model.skins[s];

        if (skin.inverseBindMatrices < 0) {
            continue;
        }

        size_t num_joints = skin.joints.size();

        if (num_joints <= 0) {
            continue;
        }

        // read the inverse bind pose matrix into a large vector of float values
        const tinygltf::Accessor& accessor = model.accessors[skin.inverseBindMatrices];
        assert(accessor.type == TINYGLTF_TYPE_MAT4);
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        const float* ptr = reinterpret_cast<const float*>(buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);

        rest_pose.resize(rest_pose.size() + num_joints);
        world_bind_transforms.resize(rest_pose.size());
        inverse_bind_matrices.resize(inverse_bind_matrices.size() + accessor.count);

        // For each joint in the skin, get the inverse bind pose matrix
        for (size_t i = 0; i < skin.joints.size(); i++) {

            size_t id = i + rest_pose.size() - num_joints;
            const tinygltf::Node& node = model.nodes[skin.joints[i]];

            joint_names.push_back(node.name);
            joint_indices.push_back(skin.joints[i]);

            Joint3D* joint_3d = new Joint3D();
            joint_3d->set_name(node.name);

            joint_nodes.push_back(joint_3d);

            if (!node.matrix.empty())
            {
                glm::mat4x4 model_matrix;

                for (int j = 0; j < 16; ++j) {
                    model_matrix[(j / 4) % 4][j % 4] = static_cast<float>(node.matrix[j]);
                }

                Transform transform = Transform::mat4_to_transform(model_matrix);
                rest_pose.set_local_transform(id, transform);
                joint_3d->set_transform(transform);
            }
            else {

                Transform transform;
                read_transform(node, transform);

                rest_pose.set_local_transform(id, transform);
                joint_3d->set_transform(transform);
            }

            int parent = -1;

            for (uint32_t j = 0; j < joint_indices.size(); j++) {
                if (joint_indices[j] != hierarchy[skin.joints[i]]) {
                    continue;
                }
                parent = j;
                break;
            }
            rest_pose.set_parent(id, parent);

            glm::mat4x4 m;

            // Get the 16 values of the inverse bind matrix and put them into a mat4
            memcpy(&m, ptr + i * 16, 16 * sizeof(float));

            inverse_bind_matrices[id] = m;

            // Set the transform into the array of transforms of the joints in the bind pose (world bind pose)
            glm::mat4x4 bind_matrix = inverse(m);
            Transform bind_transform = Transform::mat4_to_transform(bind_matrix);
            world_bind_transforms[id] = bind_transform;
        }

    }

    bind_pose = rest_pose;

    for (uint32_t i = 0; i < rest_pose.size(); ++i) {

        Transform current = world_bind_transforms[i];

        int p = bind_pose.get_parent(i);

        if (p >= 0) { // Bring into local space

            Transform parent;

            if (p < rest_pose.size()) {
                parent = world_bind_transforms[p];
            }
            else {
                const tinygltf::Node& node = model.nodes[p];
                if (!node.matrix.empty())
                {
                    glm::mat4x4 model_matrix;

                    for (int j = 0; j < 16; ++j) {
                        model_matrix[(j / 4) % 4][j % 4] = static_cast<float>(node.matrix[j]);
                    }
                    parent = Transform::mat4_to_transform(model_matrix);
                }
                else {
                    Transform transform;
                    read_transform(node, transform);
                    parent = transform;
                }
            }

            current = Transform::combine(Transform::inverse(parent), current);
        }

        bind_pose.set_local_transform(i, current);
    }

    Skeleton* skeleton = new Skeleton(rest_pose, bind_pose, joint_names, joint_indices);
    ///skeleton->set_name(skin.name);

    for (auto instance : skeleton_instances) {

        instance->set_name("Skeleton3D");
        instance->set_skeleton(skeleton, joint_nodes);

        for (auto child : instance->get_children()) {
            MeshInstance* child_instance = dynamic_cast<MeshInstance*>(child);
            assert(child_instance);
            child_instance->set_skeleton(skeleton);
        }
    }
}

void get_scalar_values(std::vector<TrackType>& out, uint32_t component_size, const tinygltf::Model& model, int accessor_idx)
{
    uint32_t size;
    const tinygltf::Accessor& accessor = model.accessors[accessor_idx];
    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

    if (accessor.type != TINYGLTF_TYPE_SCALAR) {
        component_size = accessor.type;
    }

    switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        size = component_size * sizeof(float);
        break;
        /*case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            size = component_size * sizeof(uint8_t);
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            size = component_size * sizeof(uint16_t);
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            size = component_size * sizeof(uint32_t);
            break;*/
    default:
        assert(0);
    }

    size_t vertex_idx = 0;
    size_t final_stride;

    if (buffer_view.byteStride == 0) {
        final_stride = size;
    }
    else {
        final_stride = buffer_view.byteStride;
    }

    size_t buffer_start = accessor.byteOffset + buffer_view.byteOffset;
    size_t buffer_end = size * accessor.count + buffer_start;

    size_t buffer_size = (buffer_end - buffer_start) / size;
    out.resize(buffer_size);

    size_t id = 0;

    for (size_t i = buffer_start; i < buffer_end; i += final_stride) {
        switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
        {
            switch (component_size)
            {
            case 1:
            {
                out[id++] = *(float*)&buffer.data[i];
                break;
            }
            case 3: // translate/scale
            {
                glm::vec3 v = { *(float*)&buffer.data[i] , *(float*)&buffer.data[i + 4] , *(float*)&buffer.data[i + 8] };
                out[id++] = v;
                break;
            }
            case 4: // rotation (quat)
            {
                glm::quat q = { *(float*)&buffer.data[i] , *(float*)&buffer.data[i + 4] , *(float*)&buffer.data[i + 8], *(float*)&buffer.data[i + 12] };
                out[id++] = q;
                break;
            }
            }
            break;
        }
        default:
            assert(0);
        }
    }
}

// converts a glTF animation channel into a VectorTrack or a QuaternionTrack
void track_from_channel(Track& result, const tinygltf::AnimationChannel& channel, const tinygltf::AnimationSampler& sampler, const tinygltf::Model& model)
{
    eInterpolationType interpolation;

    // make sure the Interpolation type of the track matches the cgltf_interpolation_type type of the sampler
    if (sampler.interpolation == "STEP") {
        interpolation = eInterpolationType::STEP;
    }
    else if (sampler.interpolation == "LINEAR") {
        interpolation = eInterpolationType::LINEAR;
    }
    else if (sampler.interpolation == "CUBICSPLINE") {
        interpolation = eInterpolationType::CUBIC;
    }
    else {
        assert(0);
    }

    Interpolator& interpolator = result.get_interpolator();
    interpolator.set_type(interpolation);

    // convert sampler input and output accessors into linear arrays of floating-point numbers
    std::vector<TrackType> time; // times 
    get_scalar_values(time, 1, model, sampler.input);

    uint32_t stride = result.get_stride();

    std::vector<TrackType> values; // values
    get_scalar_values(values, stride, model, sampler.output);

    size_t num_frames = time.size();

    // Resize the track to have enough room to store all the frames
    result.resize(num_frames);

    // Deal with in and out tangents for cubic interpolations
    bool is_sampler_cubic = (interpolation == eInterpolationType::CUBIC);

    // Parse the time and value arrays into frame structures
    for (size_t baseIndex = 0; baseIndex < num_frames; ++baseIndex) {

        Keyframe& frame = result[baseIndex];
        frame.time = std::get<float>(time[baseIndex]);

        // offset used to deal with cubic tracks since the input and output tangents are as large as the number of components
        size_t offset = 0;
        size_t k = is_sampler_cubic ? (baseIndex * 3) : baseIndex;

        // read input tangent (only if the sample is cubic)
        frame.in = is_sampler_cubic ? values[k + offset++] : 0.0f;
        // read the value
        frame.value = values[k + offset++];
        // read the output tangent (only if the sample is cubic)
        frame.out = is_sampler_cubic ? values[k + offset++] : 0.0f;
    }
}

void parse_model_animations(const tinygltf::Model& model, std::vector<SkeletonInstance3D*> skeleton_instances, AnimationPlayer* player)
{
    const std::vector<tinygltf::Animation>& animations = model.animations;
    size_t num_animations = animations.size();

    for (size_t i = 0; i < num_animations; ++i) {

        const tinygltf::Animation& animation = animations[i];

        SkeletonInstance3D* skeleton_instance = nullptr;
        Skeleton* skeleton = nullptr;

        Animation* new_animation = new Animation();
        new_animation->set_type(eAnimationType::ANIMATION_TYPE_SIMPLE);

        if (skeleton_instances.size() > 0) {

            for (SkeletonInstance3D* instance : skeleton_instances) {

                const std::vector<uint32_t>& indices = instance->get_skeleton()->get_joint_indices();

                for (size_t j = 0; j < animation.channels.size(); j++) {
                    for (size_t s = 0; s < indices.size(); s++) {
                        if (indices[s] != animation.channels[j].target_node)
                            continue;
                        skeleton_instance = instance;
                        skeleton = instance->get_skeleton();
                        break;
                    }
                    if (skeleton)
                        break;
                }
            }

            if (skeleton) {
                new_animation->set_type(eAnimationType::ANIMATION_TYPE_SKELETON);
            }
        }

        // each channel of a glTF file is an animation track
        size_t num_channels = animation.channels.size();

        for (size_t j = 0; j < num_channels; ++j) {

            const tinygltf::AnimationChannel& channel = animation.channels[j];
            const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];

            int node_id = -1;

            if (skeleton) {
                const std::vector<uint32_t>& indices = skeleton->get_joint_indices();

                //Convert the id node to skeleton joint id
                for (uint32_t id = 0; id < indices.size(); id++) {
                    if (indices[id] != channel.target_node)
                        continue;
                    node_id = id;
                    break;
                }
            }

            Track* track = new_animation->add_track(node_id);

            eTrackType type = eTrackType::TYPE_UNDEFINED;

            if (channel.target_path == "translation") {
                type = eTrackType::TYPE_POSITION;
            }
            else if (channel.target_path == "scale") {
                type = eTrackType::TYPE_SCALE;
            }
            else if (channel.target_path == "rotation") {
                type = eTrackType::TYPE_ROTATION;
            }
            else if (channel.target_path == "weight") {
                type = eTrackType::TYPE_FLOAT;
            }

            const tinygltf::Node& node = model.nodes[channel.target_node];

            std::string track_name = node.name + "/" + channel.target_path;

            track->set_type(type);
            track->set_name(track_name);

            Node3D* scene_parent = player->get_parent<Node3D*>();

            // Check if it's a joint and has a skeleton and get the full path..
            if (skeleton && node_id >= 0) {

                std::string parent_names = "";
                Node3D* _node = skeleton_instance;

                while (_node->get_parent<Node3D*>() != scene_parent) {
                    _node = _node->get_parent<Node3D*>();
                    parent_names = _node->get_name() + "/" + parent_names;
                }

                track->set_path(parent_names + skeleton_instance->get_name() + "/" + track_name);
            }
            else {
                // track->set_path("/" + channel.target_path);
                track->set_path(scene_parent->find_path(node.name) + channel.target_path);
            }

            track_from_channel(*track, channel, sampler, model);
        }

        new_animation->set_name(animation.name);
        new_animation->recalculate_duration();

        RendererStorage::register_animation(new_animation->get_name(), new_animation);

        /*if (i == 0) {
            player->play(new_animation->get_name());
        }*/
    }
}

bool GltfParser::parse(const char* file_path, std::vector<Node*>& entities, uint32_t flags)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    std::filesystem::path path = std::filesystem::path(file_path);

    if (path.extension() == ".gltf")
    {
        if (!loader.LoadASCIIFromFile(&model, &err, &warn, file_path)) {
            spdlog::error("Could not load \"{}\": {}", file_path, err);
            return false;
        }
    }
    else
    {
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, file_path)) {
            spdlog::error("Could not load binary \"{}\": {}", file_path, err);
            return false;
        }
    }

    if (!warn.empty()) {
        spdlog::warn(warn);
    }

    if (!err.empty()) {
        spdlog::error(err);
    }

    const tinygltf::Scene* gltf_scene = nullptr;

    if (model.defaultScene >= 0) {
        gltf_scene = &model.scenes[model.defaultScene];
    }
    else {
        gltf_scene = &model.scenes[0];
    }

    std::map<std::string, Node3D*> loaded_nodes;
    std::map<std::string, uint32_t> name_repeats;
    std::map<int, int> hierarchy;

    std::vector<SkeletonInstance3D*> skeleton_instances;

    int entity_name_idx = 0;

    bool fill_surface_data = (flags & PARSE_GLTF_FILL_SURFACE_DATA);

    std::filesystem::path path_filename = path.replace_extension().filename();

    Node3D* scene_root = root ? root : new Node3D();
    if (!root) {
        scene_root->set_name(path_filename.string() + "_root");
        entities.push_back(scene_root);
    }

    for (size_t i = 0; i < gltf_scene->nodes.size(); ++i)
    {
        uint32_t node_id = gltf_scene->nodes[i];

        assert(node_id >= 0 && node_id < model.nodes.size());

        Node3D* entity = create_node_entity(node_id, model, loaded_nodes, name_repeats);

        process_node_hierarchy(model, -1, node_id, scene_root, entity, hierarchy, skeleton_instances);

        parse_model_nodes(model, -1, node_id, scene_root, entity, loaded_nodes, name_repeats, texture_cache, mesh_cache, hierarchy, skeleton_instances, fill_surface_data);
    }

    if (model.skins.size()) {
        parse_model_skins(scene_root, model, loaded_nodes, hierarchy, skeleton_instances);
    }

    if (model.animations.size()) {
        AnimationPlayer* player = new AnimationPlayer("Animation Player");
        //gltf_root->add_child(player);
        std::vector<Node*>& nodes = scene_root->get_children();
        player->set_parent(scene_root);
        nodes.insert(nodes.begin(), player);

        parse_model_animations(model, skeleton_instances, player);
    }

    // Clean unused nodes

    for (auto instance : skeleton_instances) {
        auto& joint_names = instance->get_skeleton()->get_joint_names();

        for (auto& name : joint_names) {

            if (!loaded_nodes.contains(name)) {
                continue;
            }

            delete loaded_nodes[name];
            loaded_nodes[name] = nullptr;
        }
    }

    name_repeats.clear();
    loaded_nodes.clear();
    skeleton_instances.clear();
    hierarchy.clear();

    if (flags & PARSE_GLTF_CLEAR_CACHE) {
        clear_cache();
    }

    root = nullptr;

    return true;
}

void GltfParser::clear_cache()
{
    mesh_cache.clear();
    texture_cache.clear();
}
