#include "color_picker_2d.h"

#include "core/managers/xr/xr_manager.h"

#include "framework/ui/io.h"

#include "shaders/ui/ui_color_picker.wgsl.gen.h"

ColorPicker2D::ColorPicker2D(const std::string& sg, const Color& c) :
        ColorPicker2D(sg, { 0.0f, 0.0f }, glm::vec2(PICKER_SIZE), c, 0) {}

ColorPicker2D::ColorPicker2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size, const Color& c, uint32_t flags) :
        Panel2D(sg, pos, size, flags, c)
{
    class_type = Node2DClassType::COLOR_PICKER;

    Material* material = new Material();
    material->set_type(MATERIAL_UI);
    material->set_is_2D(true);
    material->set_cull_type(CULL_BACK);
    material->set_transparency_type(ALPHA_BLEND);
    material->set_priority(class_type);
    material->set_depth_read_write(false);
    material->set_shader(RenderStorage::get_singleton()->get_singleton()->get_shader_from_source(shaders::ui_color_picker::source, shaders::ui_color_picker::path, shaders::ui_color_picker::libraries, material));

    color = Color(rgb2hsv(color), 1.0f);

    quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

    RenderStorage::get_singleton()->register_ui_widget(material->get_shader_ref(), quad_mesh, ui_data, 3, true);

    Node::bind(name + "@changed", [&](const std::string& signal, Color color) {
        glm::vec3 new_color = rgb2hsv(glm::pow(glm::vec3(color), glm::vec3(1.0f / 2.2f)));
        this->color = glm::vec4(new_color, 1.0f);
    });
}

void ColorPicker2D::update(float delta_time)
{
    update_scroll_view();

    if (!visibility) {
        return;
    }

    sInputData data = get_input_data();

    bool hovered = data.is_hovered;

    glm::vec2 local_mouse_pos = data.local_position;
    local_mouse_pos /= size;
    local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1
    float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

    // Update hover
    hovered &= (dist < 1.f);

    if (hovered) {
        IO::push_input(this, data);
    } else {
        changing_hue = changing_sv = false;
    }

    // Update uniforms
    ui_data.flags &= ~(UI_DATA_HOVERED | UI_DATA_PRESSED | UI_DATA_SELECTED);
    ui_data.data_vec = color;

    on_hover = false;

    update_ui_data();

    Node2D::update(delta_time);
}

bool ColorPicker2D::on_input(sInputData data)
{
    IO::set_hover(this, data);

    glm::vec2 local_mouse_pos = data.local_position;
    local_mouse_pos /= size;
    local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1

    float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

    if (data.is_pressed) {
        // Change HUE
        if ((changing_hue && dist > 0.1f) || (dist > (1.0f - ring_thickness) && !changing_sv)) {
            color.r = fmod(glm::degrees(atan2f(local_mouse_pos.y, local_mouse_pos.x)), 360.f);
            changing_hue = true;
        }

        // Update Saturation, Value
        else if (!changing_hue) {
            glm::vec2 uv = local_mouse_pos;
            glm::vec2 sv = uv_to_saturation_value(uv);
            color = { color.r, sv.x, sv.y, 1.0f };
            changing_sv = true;
        }

        // Send the signal using the final color
        glm::vec3 new_color = glm::pow(hsv2rgb(color), glm::vec3(2.2f));
        Node::emit_signal(name, glm::vec4(new_color, 1.0));

        on_pressed();
    }

    if (data.was_released) {
        Node::emit_signal(name + "@released", color);
        changing_hue = changing_sv = false;
    }

    if (data.was_hovered) {
        XR_MANAGER->vibrate_hand(HAND_RIGHT, HOVER_HAPTIC_AMPLITUDE, HOVER_HAPTIC_DURATION);
    }

    // Update uniforms
    ui_data.flags |= UI_DATA_HOVERED;
    ui_data.data_vec = color;

    on_hover = true;

    update_ui_data();

    return true;
}

glm::vec2 ColorPicker2D::uv_to_saturation_value(const glm::vec2& uvs)
{
    float angle = glm::radians(color.r);
    glm::vec2 v1 = { cos(angle), sin(angle) };
    angle += glm::radians(120.f);
    glm::vec2 v2 = { cos(angle), sin(angle) };
    angle += glm::radians(120.f);
    glm::vec2 v3 = { cos(angle), sin(angle) };

    v1 *= (1.0f - ring_thickness);
    v2 *= (1.0f - ring_thickness);
    v3 *= (1.0f - ring_thickness);

    glm::vec2 base_s = v1 - v3;
    glm::vec2 base_v = base_s * 0.5f - (v2 - v3);
    glm::vec2 base_o = v3 - base_v;
    glm::vec2 p = uvs - base_o;
    float s = dot(p, base_s) / pow(length(base_s), 2.f);
    float v = dot(p, base_v) / pow(length(base_v), 2.f);
    s -= 0.5f;
    s /= v;
    s += 0.5f;

    return glm::clamp(glm::vec2(s, v), glm::vec2(0.f), glm::vec2(1.f));
}
