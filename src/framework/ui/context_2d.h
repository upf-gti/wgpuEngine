#pragma once

#include "includes.h"

#include "glm/vec2.hpp"

class Node2D;

namespace ui {

    class Context2D {

        static Node2D* focused;
        static Node2D* hovered;

        static glm::vec2 xr_position;

    public:

        static void set_xr_position(const glm::vec2& p) { xr_position = p; };

        static void set_focus(Node2D* node);
        static void set_hover(Node2D* node, const glm::vec2& p);

        static void blur() {
            focused = nullptr;
            hovered = nullptr;
        }

        static bool is_hover_disabled();

        static bool is_focus_type(uint32_t type);
        static bool equals_focus(Node2D* node);
        static bool any_focus();

        static bool any_hover();

        static Node2D* get_focus() { return focused; }
        static Node2D* get_hover() { return hovered; }

        static const glm::vec2& get_xr_position() { return xr_position; }
    };
}
