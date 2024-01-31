#pragma once

#include "glm/glm.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include "framework/utils/utils.h"

class Shader;
class Texture;

enum eMaterialFlags {
    MATERIAL_COLOR       = 1 << 0,
    MATERIAL_DIFFUSE     = 1 << 1,
    MATERIAL_TRANSPARENT = 1 << 2,
    MATERIAL_ALPHA_MASK  = 1 << 3,
    MATERIAL_UI          = 1 << 4,
    MATERIAL_PBR         = 1 << 5
};

struct Material
{
    Shader* shader = nullptr;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    Texture* diffuse_texture = nullptr;
    Texture* metallic_roughness_texture = nullptr;
    Texture* normal_texture = nullptr;
    Texture* emissive_texture = nullptr;

    float roughness = 1.0f;
    float metalness = 0.0f;
    glm::vec3 emissive = {};
    float alpha_mask = 0.5f;

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
            && roughness == other.roughness
            && metalness == other.metalness
            && emissive == other.emissive
            && alpha_mask == other.alpha_mask
            && priority == other.priority);
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
        std::size_t h7 = hash<float>()(k.roughness);
        std::size_t h8 = hash<float>()(k.metalness);
        std::size_t h9 = hash<glm::vec3>()(k.emissive);
        std::size_t h10 = hash<float>()(k.alpha_mask);
        std::size_t h11 = hash<uint8_t>()(k.priority);

        std::size_t seed = 0;
        hash_combine(seed, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        return seed;
    }
};
