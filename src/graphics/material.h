#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

class Shader;
class Texture;

enum eMaterialFlags {
    MATERIAL_COLOR       = 1 << 0,
    MATERIAL_DIFUSSE     = 1 << 1,
    MATERIAL_TRANSPARENT = 1 << 2,
    MATERIAL_UI          = 1 << 3
};

struct Material
{
    Shader* shader = nullptr;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    Texture* diffuse = nullptr;
    uint8_t type = 0;

    // Don't take transparency into account for now
    bool operator==(const Material& other) const
    {
        return (shader == other.shader
            && color == other.color
            && diffuse == other.diffuse);
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

        // Use bit shifting for hash
        // TODO: improve hash function
        return ((hash<void*>()(k.shader)
            ^ (hash<glm::vec4>()(k.color) << 1)) >> 1)
            ^ (hash<void*>()(k.diffuse) << 1);
    }
};
