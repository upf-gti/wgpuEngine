#pragma once

#include "includes.h"

#include "framework/nodes/node.h"

class Node2D;
class MeshInstance3D;
class Viewport3D;

namespace ui {
    class Text2D;
}

class IO {

    struct XrKey {
        std::string label = "";
        glm::vec2 position = { 0.0f, 0.0f };
        uint32_t flags = 0;
        std::string texture_path = "";
        glm::vec2 size = glm::vec2(42.0f);
        bool bind_event = true;
    };

    struct XrKeyboardState {
        bool caps = false;
        bool caps_locked = false;
        bool symbols = false;
        std::string input = ":";
        ui::Text2D* text = nullptr;

        const std::string& get_input() { return input; }
        void clear_input();
        void push_char(char c);
        void remove_char();
        void reset();

        void toggle_caps();
        void toggle_caps_lock();
        void toggle_symbols();
        void disable_caps();
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

    static void create_keyboard_letters_layout(std::vector<IO::XrKey>& keys, float start_x, float start_y, float margin);
    static void create_keyboard_symbols_layout(std::vector<IO::XrKey>& keys, float start_x, float start_y, float margin);

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
