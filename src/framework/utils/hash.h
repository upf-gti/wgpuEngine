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
        std::size_t h16 = hash<uint8_t>()(k.depth_read);
        std::size_t h17 = hash<uint8_t>()(k.depth_write);
        std::size_t h18 = hash<uint8_t>()(k.use_skinning);

        std::size_t seed = 0;
        hash_combine(seed, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15, h16, h17, h18);
        return seed;
    }
};

struct RenderPipelineKey {
    Shader* shader;
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

        std::size_t h1 = hash<void*>()(k.shader);
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
