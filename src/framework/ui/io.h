#pragma once

#include "includes.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include <string>
#include <vector>

class Node2D;
class MeshInstance3D;
class Viewport3D;

class IO {

    struct XrKey {
        std::string label = "";
        glm::vec2 position = {};
        uint32_t flags = 0;
    };

    struct XrKeyboardState {
        bool caps = false;
    };

    static XrKeyboardState xr_keyboard_state;

    static float xr_ray_distance;

    static Node2D* focused;
    static Node2D* hovered;

    static glm::vec2 xr_position;
    static glm::vec3 xr_world_position;

    static Node2D* keyboard_2d;
    static Viewport3D* xr_keyboard;
    static bool keyboard_active;

    static std::vector<XrKey> create_keyboard_layout(float start_x, float start_y, float spacing);

public:

    static void initialize();
    static void start_frame();
    static void render();
    static void update(float delta_time);

    static void set_xr_ray_distance(float d) { xr_ray_distance = d; };
    static void set_xr_position(const glm::vec2& p) { xr_position = p; };
    static void set_xr_world_position(const glm::vec3& p) { xr_world_position = p; };

    static void set_focus(Node2D* node);
    static void set_hover(Node2D* node, const glm::vec2& p, float ray_distance = -1.0f);

    static void blur() {
        focused = nullptr;
        hovered = nullptr;
    }

    static bool is_hover_disabled();

    static bool is_focus_type(uint32_t type);
    static bool equals_focus(Node2D* node);
    static bool any_focus();

    static bool is_hover_type(uint32_t type);
    static bool is_any_hover_type(const std::vector<uint32_t>& types);
    static bool any_hover();

    static Node2D* get_focus() { return focused; }
    static Node2D* get_hover() { return hovered; }

    static const glm::vec2& get_xr_position() { return xr_position; }
    static const glm::vec3& get_xr_world_position() { return xr_world_position; }
    static const float get_xr_ray_distance() { return xr_ray_distance; }

    static void request_keyboard() { keyboard_active = true; };
    static void close_keyboard() { keyboard_active = false; };
};
