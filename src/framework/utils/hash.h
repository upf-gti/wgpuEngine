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

inline void hash_combine(std::size_t& seed, std::vector<std::size_t> vs)
{
    for (const auto& v : vs) {
        hash_combine(seed, v);
    }
}

struct RenderPipelineKey {
    const Shader* shader;
    RenderPipelineDescription description;
    WGPUPipelineLayout pipeline_layout;

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

        std::size_t h_mult[MAX_ATTACHMENT_COUNT * 3u] = { 0 };
        for (uint8_t i = 0u; i < k.description.sample_count; i++) {
            h_mult[i * 3] = hash<uint32_t>()(static_cast<uint32_t>(k.description.color_targets[i].format));
            h_mult[i * 3 + 1] = hash<uint32_t>()(static_cast<uint32_t>(k.description.color_targets[i].writeMask));
            h_mult[i * 3 + 2] = hash<void const*>()(k.description.color_targets[i].blend);
        }
        
        std::size_t h5 = hash<uint32_t>()(static_cast<uint32_t>(k.description.cull_mode));
        std::size_t h6 = hash<uint32_t>()(static_cast<uint32_t>(k.description.topology));
        std::size_t h7 = hash<uint32_t>()(static_cast<uint32_t>(k.description.depth_read));
        std::size_t h8 = hash<uint32_t>()(static_cast<uint32_t>(k.description.depth_write));
        std::size_t h9 = hash<uint32_t>()(static_cast<uint32_t>(k.description.blending_enabled));
        std::size_t h10 = hash<uint32_t>()(static_cast<uint32_t>(k.description.topology));
        std::size_t h11 = hash<uint32_t>()(static_cast<uint32_t>(k.description.depth_compare));
        std::size_t h12 = hash<const void*>()(k.pipeline_layout);

        // lo siento....
        std::size_t seed = 0;
        hash_combine(seed, h1, h5, h6, h7, h8, h9, h10, h11, h12, h_mult[0], h_mult[1], h_mult[2], h_mult[3], h_mult[4], h_mult[5], h_mult[6], h_mult[7], h_mult[8], h_mult[9], h_mult[10], h_mult[11], h_mult[12], h_mult[13], h_mult[14] );
        return seed;
    }
};
