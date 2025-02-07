#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct sLightUniformData {
    glm::vec3 position;
    int type = 0;
    glm::vec3 color = { 0.0f, 0.0f, 0.0f };
    float intensity = 0.0f;
    glm::vec3 direction = { 0.0f, 0.0f, 0.0f };
    float range = 0.0f;
    glm::vec2 dummy = { 0.0f, 0.0f };
    // spots
    float inner_cone_cos = 0.0f;
    float outer_cone_cos = 0.0f;
};

enum sUIDataFlags {
    UI_DATA_HOVERED = 1 << 0,
    UI_DATA_PRESSED = 1 << 1,
    UI_DATA_SELECTED = 1 << 2,
    UI_DATA_COLOR_BUTTON = 1 << 3,
    UI_DATA_DISABLED = 1 << 4
};

struct sUIData {
    uint32_t flags = 0u;
    float hover_time = 0.0f;
    float aspect_ratio = 1.f;
    float data_value = 0.0f; // generic data, slider value, group num items, combo buttons use this prop by now to the index in combo..

    glm::vec3 data_vec = glm::vec3(1.f); // picker color, slider range(+slider_type at .z)..
    float clip_range = 0.f; // inside/outside the node parent.. -1..1

    glm::vec2 xr_size = { 0.0f, 0.0f };
    glm::vec2 xr_position = { 0.0f, 0.0f };
};
