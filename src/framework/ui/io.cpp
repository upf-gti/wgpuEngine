#include "io.h"

#include "framework/nodes/ui.h"
#include "framework/nodes/ui.h"
#include "framework/nodes/viewport_3d.h"
#include "framework/scene/parse_scene.h"
#include "framework/input.h"

#include "graphics/renderer.h"

#include "spdlog/spdlog.h"

float IO::xr_ray_distance = 0.0f;

Node2D* IO::focused = nullptr;
Node2D* IO::hovered = nullptr;

glm::vec2 IO::xr_position = { 0.0f, 0.0f };
glm::vec3 IO::xr_world_position = { 0.0f, 0.0f, 0.0f };

Node2D* IO::keyboard_2d = nullptr;
Viewport3D* IO::xr_keyboard = nullptr;
bool IO::keyboard_active = false;
IO::XrKeyboardState IO::xr_keyboard_state;

void IO::initialize()
{
    keyboard_active = true;

    // Build XR Keyboard
    {
        std::string name = "xr_keyboard";

        uint32_t max_row_count = 10;
        uint32_t max_col_count = 3;

        float button_size = 42.0f;
        float button_margin = 4.0f;
        float full_size = button_size + button_margin;

        const std::vector<IO::XrKey>& keys = create_keyboard_layout(full_size * 0.5f, full_size * 0.33f, full_size);

        keyboard_2d = new Node2D(name, { 512.0f, 64.0f }, { 1.0f, 1.0f });

        ui::XRPanel* root = new ui::XRPanel(name + "@background", Color(0.0f, 0.0f, 0.0f, 0.8f), { 0.0f, 0.f }, glm::vec2(button_size * (max_row_count + 2u), button_size * (max_col_count + 1u)));
        keyboard_2d->add_child(root);

        for (const IO::XrKey& key : keys) {
            root->add_child(new ui::TextureButton2D(key.label, "", key.flags | ui::SKIP_NAME, key.position, glm::vec2(button_size)));
            Node::bind(key.label, [](const std::string& sg, void* data) {
                if (sg == "Shift") {
                    xr_keyboard_state.caps = !xr_keyboard_state.caps;
                }
                else {
                    spdlog::info("KEYDOWN!! {}", xr_keyboard_state.caps ? std::string(1, std::toupper(sg[0])) : sg);
                }
            });
        }
    }
}

void IO::start_frame()
{
    set_xr_ray_distance(-1.0f);
}

void IO::render()
{
    if (Renderer::instance->get_openxr_available() && keyboard_active) {
        xr_keyboard->render();
    }

    // debug for flat screen!! remove later
    keyboard_2d->render();
}

void IO::update(float delta_time)
{
    auto renderer = Renderer::instance;

    if (renderer->get_openxr_available() && keyboard_active) {
        glm::mat4x4 m(1.0f);
        /*glm::vec3 eye = renderer->get_camera_eye();
        glm::vec3 new_pos = eye + renderer->get_camera_front() * 1.5f;

        m = glm::translate(m, new_pos);
        m = m * glm::toMat4(get_rotation_to_face(new_pos, eye, { 0.0f, 1.0f, 0.0f }));
        m = glm::rotate(m, glm::radians(180.f), { 1.0f, 0.0f, 0.0f });*/

        xr_keyboard->set_model(m);
        xr_keyboard->update(delta_time);
    }

    // debug for flat screen!! remove later
    keyboard_2d->update(delta_time);

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

    xr_position = p;
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

std::vector<IO::XrKey> IO::create_keyboard_layout(float start_x, float start_y, float spacing)
{
    std::vector<IO::XrKey> keys;

    std::string first_row = "qwertyuiop";
    for (size_t i = 0; i < first_row.size(); ++i) {
        keys.push_back({ std::string(1, first_row[i]), glm::vec2(start_x + i * spacing, start_y) });
    }

    std::string second_row = "asdfghjkl";
    for (size_t i = 0; i < second_row.size(); ++i) {
        keys.push_back({ std::string(1, second_row[i]), glm::vec2(start_x + i * spacing + spacing * 0.50f, start_y + spacing) });
    }

    std::string third_row = "zxcvbnm";
    for (size_t i = 0; i < third_row.size(); ++i) {
        keys.push_back({ std::string(1, third_row[i]), glm::vec2(start_x + (i + 1) *spacing + spacing, start_y + spacing * 2.0f) });
    }

    // Additional rows (space, enter, backspace)
    keys.push_back({ "Shift", glm::vec2(start_x + spacing, start_y + spacing * 2.0f), ui::ALLOW_TOGGLE });

    return keys;
}
