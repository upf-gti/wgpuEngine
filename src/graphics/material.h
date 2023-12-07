#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "utils.h"

class Shader;
class Texture;

enum eMaterialFlags {
    MATERIAL_COLOR       = 1 << 0,
    MATERIAL_DIFFUSE     = 1 << 1,
    MATERIAL_TRANSPARENT = 1 << 2,
    MATERIAL_UI          = 1 << 3,
    MATERIAL_PBR         = 1 << 4
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
            && emissive_texture == other.emissive_texture);
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

        std::size_t seed = 0;
        hash_combine(seed, h1, h2, h3, h4, h5, h6);
        return seed;
    }
};
