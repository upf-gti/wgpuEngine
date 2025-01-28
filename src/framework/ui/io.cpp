#include "io.h"

#include "framework/nodes/panel_2d.h"
#include "framework/nodes/node_2d.h"
#include "framework/nodes/button_2d.h"
#include "framework/nodes/viewport_3d.h"
#include "framework/parsers/parse_scene.h"
#include "framework/input.h"

#include "graphics/renderer.h"

#include <algorithm>
#include "imgui.h"
#include "spdlog/spdlog.h"

bool IO::want_capture_input = false;

float IO::xr_ray_distance = 0.0f;
float IO::last_xr_ray_distance = 0.0f;

Node2D* IO::focused = nullptr;
Node2D* IO::hovered = nullptr;

glm::vec2 IO::xr_position = { 0.0f, 0.0f };
glm::vec3 IO::xr_world_position = { 0.0f, 0.0f, 0.0f };

std::vector<std::pair<Node2D*, sInputData>> IO::frame_inputs;

void IO::initialize()
{
    // ...
}

void IO::start_frame()
{
    set_xr_ray_distance(-1.0f);

    want_capture_input = false;
}

void IO::end_frame()
{
    last_xr_ray_distance = xr_ray_distance;
}

void IO::update(float delta_time)
{
    if (!frame_inputs.size()) {

        IO::blur();
        return;
    }

    // xr: sort inputs by ray distance and later by priority
    // flat screen: sort by priority

    bool is_xr = Renderer::instance->get_openxr_available();

    std::sort(frame_inputs.begin(), frame_inputs.end(), [xr = is_xr](auto& lhs, auto& rhs) {

        Node2D* lhs_node = lhs.first;
        Node2D* rhs_node = rhs.first;

        if (xr) {

            float dt = glm::abs(lhs.second.ray_distance - rhs.second.ray_distance);
            if (dt < 0.01f) {
                return lhs_node->get_class_type() < rhs_node->get_class_type();
            }

            return lhs.second.ray_distance < rhs.second.ray_distance;
        }

        return lhs_node->get_class_type() < rhs_node->get_class_type();
    });

    // call on_input functions..

    for (const auto& i : frame_inputs) {

        bool event_processed = i.first->on_input(i.second);
        if (event_processed) {
            break;
        }
    }

    frame_inputs.clear();
}

void IO::set_focus(Node2D* node)
{
    focused = node;
}

void IO::set_hover(Node2D* node, const sInputData& data)
{
    hovered = node;

    xr_ray_distance = data.ray_distance;

    xr_position = data.local_position;

    xr_world_position = data.ray_intersection;
}

void IO::blur()
{
    hovered = nullptr;

    // If actions are pressed, we are still focusing something..
    bool should_remove_focus = Renderer::instance->get_openxr_available() ?
        (Input::get_trigger_value(HAND_RIGHT) <= XR_THUMBSTICK_DEADZONE) :
        !Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);

    if (should_remove_focus) {
        focused = nullptr;
    }
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

bool IO::is_focused(Node2D* node)
{
    return (focused == node);
}

bool IO::any_focus()
{
    return (focused != nullptr);
}

bool IO::is_hovered(Node2D* node)
{
    return (hovered == node);
}

bool IO::is_hover_type(uint32_t type, uint32_t flag)
{
    if (!any_hover()) {
        return false;
    }

    ui::Panel2D* root = dynamic_cast<ui::Panel2D*>(hovered);
    if (!root) {
        return false;
    }

    bool result = (root->get_class_type() == type);

    if (flag != 0u) {
        bool has_flag = (root->get_flags() & flag);
        result |= has_flag;
    }

    return result;
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

bool IO::get_want_capture_input()
{
    auto& io = ImGui::GetIO();
    return io.WantCaptureMouse || want_capture_input;
}

bool IO::any_hover()
{
    return (hovered != nullptr);
}

void IO::push_input(Node2D* node, sInputData data)
{
    frame_inputs.push_back({ node, data });
}
