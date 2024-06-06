#include "context_2d.h"

#include "framework/nodes/ui.h"

namespace ui {

    Node2D* Context2D::focused = nullptr;
    Node2D* Context2D::hovered = nullptr;

    glm::vec2 Context2D::xr_position = { 0.0f, 0.0f };
    glm::vec3 Context2D::xr_world_position = { 0.0f, 0.0f, 0.0f };

    void Context2D::set_focus(Node2D* node)
    {
        focused = node;
    }

    void Context2D::set_hover(Node2D* node, const glm::vec2& p)
    {
        hovered = node;

        set_xr_position(p);
    }

    bool Context2D::is_hover_disabled()
    {
        if (!hovered) {
            return false;
        }

        Button2D* button = dynamic_cast<Button2D*>(hovered);
        if (!button) {
            return false;
        }

        return button->disabled;
    }

    bool Context2D::is_focus_type(uint32_t type)
    {
        if (!any_focus()) {
            return false;
        }

        return (focused->get_class_type() == type);
    }

    bool Context2D::equals_focus(Node2D* node)
    {
        return (focused == node);
    }

    bool Context2D::any_focus()
    {
        return (focused != nullptr);
    }

    bool Context2D::any_hover()
    {
        return (hovered != nullptr);
    }
}
