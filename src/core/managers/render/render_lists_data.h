#pragma once

#include "glm/mat4x4.hpp"
#include <vector>

class Mesh;

struct RenderableData {
    Mesh* mesh = nullptr;
    glm::mat4x4 global_matrix;
};

struct RenderableDataRef {
    RenderableData data;
    uint32_t idx = 0;
};

struct LightData {
    glm::mat4x4 view_proj;
    glm::vec3 position;
    int type = 0;
    glm::vec3 color = { 0.0f, 0.0f, 0.0f };
    float intensity = 0.0f;
    glm::vec3 direction = { 0.0f, 0.0f, 0.0f };
    float range = 0.0f;
    float shadow_bias = 0.001f;
    int cast_shadows = 0;
    float inner_cone_cos = 0.0f;
    float outer_cone_cos = 0.0f;
};

struct LightDataRef {
    LightData data;
    uint32_t idx = 0;
};

struct SplatData {
    std::vector<glm::vec4> positions;
    std::vector<glm::vec4> colors;
    std::vector<glm::quat> rotations;
    std::vector<glm::vec4> scales;
};

struct SplatsDataRef {
    SplatData data;
    uint32_t idx = 0;
};
