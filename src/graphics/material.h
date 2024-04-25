#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>

class Shader;
class Texture;

enum eMaterialFlags : uint8_t {
    MATERIAL_PBR        = 1 << 0,
    MATERIAL_2D         = 1 << 1,
    MATERIAL_UI         = 1 << 2
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

    bool depth_read = true;
    bool depth_write = true;
    bool use_skinning = false;

    eTransparencyType transparency_type = ALPHA_OPAQUE;
    eTopologyType topology_type = TOPOLOGY_TRIANGLE_LIST;
    eCullType cull_type = CULL_BACK;

    uint8_t flags = 0;
    uint8_t priority = 10;

    std::string name = "";

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
            && depth_read == other.depth_read
            && depth_write == other.depth_write
            && use_skinning == other.use_skinning);
    }
};
