#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

class Shader;
class Texture;

struct Material
{
    Shader* shader = nullptr;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    Texture* diffuse = nullptr;
    bool transparent = false;

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
