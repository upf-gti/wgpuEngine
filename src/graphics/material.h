#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "framework/resources/resource.h"

#include <string>

class Shader;
class Texture;

enum eMaterialType {
    MATERIAL_PBR,
    MATERIAL_UNLIT,
    MATERIAL_UI
};

enum eTransparencyType {
    ALPHA_OPAQUE,
    ALPHA_BLEND,
    ALPHA_MASK,
    ALPHA_HASH // TODO
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

struct Material : public Resource
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
    bool is_2D = false;

    eTransparencyType transparency_type = ALPHA_OPAQUE;
    eTopologyType topology_type = TOPOLOGY_TRIANGLE_LIST;
    eCullType cull_type = CULL_BACK;

    eMaterialType type = MATERIAL_PBR;
    uint8_t priority = 10;

    std::string name = "";

};
