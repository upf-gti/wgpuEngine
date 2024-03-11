#pragma once

#include "framework/math.h"

#include "glm/gtx/hash.hpp"

#include "framework/utils/utils.h"

class Shader;
class Texture;

enum eMaterialFlags {
    MATERIAL_DIFFUSE    = 1 << 0,
    MATERIAL_PBR        = 1 << 1,
    MATERIAL_2D         = 1 << 2,
    MATERIAL_UI         = 1 << 3
};

enum eTransparencyType {
    ALPHA_OPAQUE,
    ALPHA_BLEND,
    ALPHA_MASK,
    ALPHA_HASH
};

enum eTopologyType {
    TOPOLOGY_POINT_LIST,
    TOPOLOGY_LINE_LIST,
    TOPOLOGY_LINE_STRIP,
    TOPOLOGY_TRIANGLE_LIST,
    TOPOLOGY_TRIANGLE_STRIP
};

enum eCullType {
    CULL_NONE,
    CULL_BACK,
    CULL_FRONT
};

struct Material
{
    Shader* shader = nullptr;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    Texture* diffuse_texture = nullptr;
    Texture* metallic_roughness_texture = nullptr;
    Texture* normal_texture = nullptr;
    Texture* emissive_texture = nullptr;
    Texture* oclussion_texture = nullptr;

    float roughness = 1.0f;
    float metalness = 0.0f;
    float occlusion = 1.0f;
    glm::vec3 emissive = {};
    float alpha_mask = 0.5f;

    bool depth_write = true;

    eTransparencyType transparency_type = ALPHA_OPAQUE;
    eTopologyType topology_type = TOPOLOGY_TRIANGLE_LIST;
    eCullType cull_type = CULL_NONE;

    uint8_t flags = 0;
    uint8_t priority = 0;

    // Don't take transparency into account for now
    bool operator==(const Material& other) const
    {
        return (shader == other.shader
            && color == other.color
            && diffuse_texture == other.diffuse_texture
            && metallic_roughness_texture == other.metallic_roughness_texture
            && normal_texture == other.normal_texture
            && emissive_texture == other.emissive_texture
            && oclussion_texture == other.oclussion_texture
            && roughness == other.roughness
            && metalness == other.metalness
            && emissive == other.emissive
            && alpha_mask == other.alpha_mask
            && priority == other.priority
            && transparency_type == other.transparency_type
            && topology_type == other.topology_type
            && cull_type == other.cull_type
            && depth_write == other.depth_write);
    }
};

template <>
struct std::hash<Material>
{
    std::size_t operator()(const Material& k) const
    {
        using std::size_t;
        using std::hash;
        using std::string;

        std::size_t h1 = hash<void*>()(k.shader);
        std::size_t h2 = hash<glm::vec4>()(k.color);
        std::size_t h3 = hash<void*>()(k.diffuse_texture);
        std::size_t h4 = hash<void*>()(k.normal_texture);
        std::size_t h5 = hash<void*>()(k.metallic_roughness_texture);
        std::size_t h6 = hash<void*>()(k.emissive_texture);
        std::size_t h7 = hash<void*>()(k.oclussion_texture);
        std::size_t h8 = hash<float>()(k.roughness);
        std::size_t h9 = hash<float>()(k.metalness);
        std::size_t h10 = hash<glm::vec3>()(k.emissive);
        std::size_t h11 = hash<float>()(k.alpha_mask);
        std::size_t h12 = hash<uint8_t>()(k.priority);
        std::size_t h13 = hash<uint8_t>()(k.transparency_type);
        std::size_t h14 = hash<uint8_t>()(k.topology_type);
        std::size_t h15 = hash<uint8_t>()(k.cull_type);
        std::size_t h16 = hash<uint8_t>()(k.depth_write);

        std::size_t seed = 0;
        hash_combine(seed, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15, h16);
        return seed;
    }
};
