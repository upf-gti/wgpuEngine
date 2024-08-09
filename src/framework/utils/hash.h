#pragma once

#include "graphics/material.h"
#include "graphics/pipeline.h"

#include "glm/gtx/hash.hpp"

// based on: https://stackoverflow.com/a/38140932
template <typename... Rest>
inline void hash_combine(std::size_t& seed, std::size_t v, const Rest... rest)
{
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

struct RenderPipelineKey {
    const Shader* shader;
    WGPUColorTargetState color_target;
    PipelineDescription description;

    bool operator==(const RenderPipelineKey& other) const;
};

template <>
struct std::hash<RenderPipelineKey>
{
    std::size_t operator()(const RenderPipelineKey& k) const
    {
        using std::size_t;
        using std::hash;
        using std::string;

        std::size_t h1 = hash<const void*>()(k.shader);
        std::size_t h2 = hash<uint32_t>()(static_cast<uint32_t>(k.color_target.format));
        std::size_t h3 = hash<uint32_t>()(static_cast<uint32_t>(k.color_target.writeMask));
        std::size_t h4 = hash<void const*>()(k.color_target.blend);
        std::size_t h5 = hash<uint32_t>()(static_cast<uint32_t>(k.description.cull_mode));
        std::size_t h6 = hash<uint32_t>()(static_cast<uint32_t>(k.description.topology));

        std::size_t seed = 0;
        hash_combine(seed, h1, h2, h3, h4, h5, h6);
        return seed;
    }
};
