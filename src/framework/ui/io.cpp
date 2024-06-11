#include "io.h"

#include "framework/nodes/ui.h"
#include "framework/scene/parse_scene.h"
#include "framework/input.h"

float IO::xr_ray_distance = 0.0f;

Node2D* IO::focused = nullptr;
Node2D* IO::hovered = nullptr;

glm::vec2 IO::xr_position = { 0.0f, 0.0f };
glm::vec3 IO::xr_world_position = { 0.0f, 0.0f, 0.0f };

void IO::initialize()
{
    // ...
}

void IO::start_frame()
{
    set_xr_ray_distance(-1.0f);
}

// Call this pre-render!!
void IO::end_frame()
{
    Node2D::process_input();
}

void IO::set_focus(Node2D* node)
{
    focused = node;
}

void IO::set_hover(Node2D* node, const glm::vec2& p, float ray_distance)
{
    hovered = node;

    xr_ray_distance = ray_distance;

    set_xr_position(p);
}

bool IO::is_hover_disabled()
{
    if (!hovered) {
        return false;
    }

    ui::Button2D* button = dynamic_cast<ui::Button2D*>(hovered);
    if (!button) {
        return false;
    }

    return button->disabled;
}

bool IO::is_focus_type(uint32_t type)
{
    if (!any_focus()) {
        return false;
    }

    return (focused->get_class_type() == type);
}

bool IO::equals_focus(Node2D* node)
{
    return (focused == node);
}

bool IO::any_focus()
{
    return (focused != nullptr);
}

bool IO::is_hover_type(uint32_t type)
{
    if (!any_hover()) {
        return false;
    }

    return (hovered->get_class_type() == type);
}

bool IO::is_any_hover_type(const std::vector<uint32_t>& types)
{
    if (!any_hover()) {
        return false;
    }

    for (uint32_t type : types) {
        if (hovered->get_class_type() == type) {
            return true;
        }
    }

    return false;
}

bool IO::any_hover()
{
    return (hovered != nullptr);
}
