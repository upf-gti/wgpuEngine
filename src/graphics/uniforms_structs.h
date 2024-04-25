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

struct sUIData {
    // Common
    float is_hovered = 0.f;
    // Groups
    float num_group_items = 2; // combo buttons use this prop by now to the index in combo
    // Buttons
    float is_selected = 0.f;
    float is_color_button = 0.f;

    // Color Picker
    glm::vec3 picker_color = glm::vec3(1.f);
    float dummy = 0.f;

    // To keep rgb if icon has colors...
    float keep_rgb = 0.f;
    // Slider
    float slider_value = 0.f;
    float slider_max = 1.0f;
    // Disable buttons to use them as group icons
    float is_button_disabled = 0.f;
};
