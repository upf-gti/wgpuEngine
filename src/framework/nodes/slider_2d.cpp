#include "slider_2d.h"

#include <cctype>

#include "engine/engine.h"

#include "framework/math/intersections.h"
#include "framework/input.h"
#include "framework/nodes/text.h"
#include "framework/camera/camera.h"
#include "framework/utils/utils.h"
#include "framework/ui/io.h"

#include "graphics/renderer.h"
#include "graphics/webgpu_context.h"

#include "glm/gtx/easing.hpp"
#include "glm/gtx/compatibility.hpp"

#include "shaders/ui/ui_slider.wgsl.gen.h"

namespace ui {

    Slider2D::Slider2D(const std::string& sg, const sSliderDescription& desc)
        : Panel2D(sg, desc.position, desc.size, desc.flags)
    {
        data = desc.p_data;
    }

    void Slider2D::set_disabled(bool new_disabled)
    {
        disabled = new_disabled;
        ui_data.is_button_disabled = disabled;
        update_ui_data();
    }

    template <typename T>
    void Slider2D::process_wheel_joystick(T wheel_multiplier, T joystick_multiplier)
    {
        // Send signal on move thumbstick or using wheel hovering the text
        T dt = static_cast<T>(Input::get_thumbstick_value(HAND_RIGHT).y) * joystick_multiplier;

        if (glm::abs(dt) > 0.01f) {
            Node::emit_signal(name + "@stick_moved", dt);
        }

        T wheel_dt = static_cast<T>(Input::get_mouse_wheel_delta()) * wheel_multiplier;

        if (glm::abs(wheel_dt) > 0.01f) {
            Node::emit_signal(name + "@stick_moved", wheel_dt);
        }
    }

    /*
    *	FloatSlider
    */

    FloatSlider2D::FloatSlider2D(const std::string& sg, const sSliderDescription& desc)
        : Slider2D(sg, desc), original_value(desc.fvalue), current_value(desc.fvalue), min_value(desc.fvalue_min), max_value(desc.fvalue_max), precision(desc.precision) {

        bool is_horizontal = (mode == SliderMode::HORIZONTAL);

        parameter_flags |= DBL_CLICK;

        this->class_type = is_horizontal ? Node2DClassType::HSLIDER : Node2DClassType::VSLIDER;
        this->mode = mode;

        disabled = parameter_flags & DISABLED;

        ui_data.num_group_items = is_horizontal ? 2.f : 1.f;
        ui_data.slider_max = max_value;
        ui_data.slider_min = min_value;

        if (parameter_flags & CURVE_INV_POW) {
            ui_data.slider_max = 1.0f / glm::pow(2.0f, max_value);
            ui_data.slider_min = 1.0f / glm::pow(2.0f, min_value);
        }

        ui_data.is_button_disabled = disabled;

        this->size = glm::vec2(size.x * ui_data.num_group_items, size.y);

        ui_data.aspect_ratio = this->size.x / this->size.y;

        current_value = glm::clamp(current_value, min_value, max_value);

        Material* material = new Material();
        material->set_color(colors::WHITE);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);
        material->set_diffuse_texture(desc.path.size() > 0 ? RendererStorage::get_texture(desc.path, TEXTURE_STORAGE_UI) : nullptr);
        material->set_depth_read_write(false);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_slider::source, shaders::ui_slider::path, shaders::ui_slider::libraries, material));

        Surface* quad_surface = quad_mesh->get_surface(0);
        quad_surface->create_quad(this->size.x, this->size.y, true);

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);

        if (desc.p_data) {

            float* f_data = reinterpret_cast<float*>(data);
            current_value = *f_data;

            Node::bind(name, (FuncFloat)[&](const std::string& signal, float value) {
                if (data) {
                    float* f_data = reinterpret_cast<float*>(data);
                    *f_data = current_value;
                }
            });
        }

        Node::bind(name + "@changed", (FuncFloat)[&](const std::string& signal, float value) {
            set_value(value);
        });

        Node::bind(name + "@stick_moved", (FuncFloat)[&](const std::string& signal, float dt) {
            set_value(current_value + dt);
            Node::emit_signal(name, current_value);
            Node::emit_signal(name + "_x", current_value);
        });

        // Text labels (only if slider is enabled)
        {
            float text_scale = 18.0f;

            if (!(parameter_flags & SKIP_NAME)) {
                std::string pretty_name = name;
                to_camel_case(pretty_name);
                text_2d = new Text2D(pretty_name, { 0.f, size.y }, text_scale, TEXT_CENTERED);
                add_child(text_2d);
            }

            if (!disabled && !(parameter_flags & SKIP_VALUE)) {
                std::string value_as_string = value_to_string();
                text_2d_value = new Text2D(value_as_string, { 0.0f, -text_scale }, text_scale, TEXT_CENTERED);
                add_child(text_2d_value);
            }
        }
    }

    void FloatSlider2D::update(float delta_time)
    {
        timer += delta_time;

        update_scroll_view();

        if (!visibility)
            return;

        sInputData data = get_input_data();

        if (!disabled && data.is_hovered) {
            IO::push_input(this, data);
        }

        // Update uniforms
        ui_data.hover_info.x = 0.0f;
        ui_data.hover_info.y = 0.0f;
        ui_data.slider_value = current_value;

        // Only appear on hover if slider is enabled..
        if (!disabled && text_2d) {
            text_2d->set_visibility(false);
        }

        on_hover = false;

        update_ui_data();

        Node2D::update(delta_time);
    }

    bool FloatSlider2D::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        if (text_2d) {
            text_2d->set_visibility(true);
        }

        if (data.is_pressed) {
            float real_size = (mode == HORIZONTAL ? size.x : size.y);
            float local_point = (mode == HORIZONTAL ? data.local_position.x : data.local_position.y);
            // This is at range 0..1
            current_value = glm::clamp(local_point / real_size, 0.f, 1.f);
            // Set in range min-max
            current_value = remap_range(current_value, 0.0f, 1.0f, min_value, max_value);
            // Make sure it reaches min, max values
            if (std::abs(current_value - min_value) < 1e-4f) current_value = min_value;
            else if (std::abs(current_value - max_value) < 1e-4f) current_value = max_value;

            if (text_2d_value) {
                text_2d_value->text_entity->set_text(value_to_string());
            }

            // Use curve if needed
            if (parameter_flags & CURVE_INV_POW) {
                current_value = 1.0f / glm::pow(2.0f, current_value);
            }

            Node::emit_signal(name, current_value);
        }

        if (data.was_released) {

            // if enters here, it means it's a double click, so set original value
            if (on_pressed()) {
                set_value(original_value);
                Node::emit_signal(name, current_value);
            }
        }

        if (data.was_hovered) {
            timer = 0.f;
            Engine::instance->vibrate_hand(HAND_RIGHT, HOVER_HAPTIC_AMPLITUDE, HOVER_HAPTIC_DURATION);
        }

        process_wheel_joystick<float>(1.0f / (powf(10.0f, static_cast<float>(precision))), 0.25f);

        on_hover = true;

        hover_factor = glm::cubicEaseOut(glm::clamp(timer / 0.2f, 0.0f, 1.0f));

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.hover_info.y = glm::clamp(hover_factor, 0.0f, 1.0f);
        ui_data.slider_value = current_value;

        update_ui_data();

        return true;
    }

    void FloatSlider2D::set_value(float new_value)
    {
        current_value = glm::clamp(new_value, min_value, max_value);
        if (text_2d_value) {
            text_2d_value->text_entity->set_text(value_to_string());
        }
    }

    std::string FloatSlider2D::value_to_string()
    {
        // Use default 0..1 range for showing the user
        float value = remap_range(current_value, min_value, max_value, 0.0f, 1.0f);

        if (parameter_flags & USER_RANGE) {
            float fprecision = 1.0f / glm::pow(10.0f, precision);
            value = std::roundf(current_value / fprecision) * fprecision;
        }

        std::string s = std::to_string(value);
        size_t idx = s.find('.') + (precision > 0 ? 1 : 0);
        return s.substr(0, idx + precision);
    }

    /*
    *	IntSlider
    */

    IntSlider2D::IntSlider2D(const std::string& sg, const sSliderDescription& desc)
        : Slider2D(sg, desc), original_value(desc.ivalue), current_value(desc.ivalue), min_value(desc.ivalue_min), max_value(desc.ivalue_max) {

        bool is_horizontal = (mode == SliderMode::HORIZONTAL);

        parameter_flags |= DBL_CLICK;

        this->class_type = is_horizontal ? Node2DClassType::HSLIDER : Node2DClassType::VSLIDER;
        this->mode = mode;

        disabled = parameter_flags & DISABLED;

        ui_data.num_group_items = is_horizontal ? 2.f : 1.f;
        ui_data.slider_max = static_cast<float>(max_value);
        ui_data.slider_min = static_cast<float>(min_value);

        if (parameter_flags & CURVE_INV_POW) {
            ui_data.slider_max = 1.0f / glm::pow(2.0f, static_cast<float>(max_value));
            ui_data.slider_min = 1.0f / glm::pow(2.0f, static_cast<float>(min_value));
        }

        ui_data.is_button_disabled = disabled;

        this->size = glm::vec2(size.x * ui_data.num_group_items, size.y);

        ui_data.aspect_ratio = this->size.x / this->size.y;

        current_value = glm::clamp(current_value, min_value, max_value);

        Material* material = new Material();
        material->set_color(colors::WHITE);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);
        material->set_diffuse_texture(desc.path.size() > 0 ? RendererStorage::get_texture(desc.path, TEXTURE_STORAGE_UI) : nullptr);
        material->set_depth_read_write(false);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_slider::source, shaders::ui_slider::path, shaders::ui_slider::libraries, material));

        Surface* quad_surface = quad_mesh->get_surface(0);
        quad_surface->create_quad(this->size.x, this->size.y, true);

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);

        if (desc.p_data) {

            int* i_data = reinterpret_cast<int*>(data);
            current_value = *i_data;

            Node::bind(name, (FuncInt)[&](const std::string& signal, int value) {
                if (data) {
                    int* i_data = reinterpret_cast<int*>(data);
                    *i_data = current_value;
                }
            });
        }

        Node::bind(name + "@changed", (FuncInt)[&](const std::string& signal, int value) {
            set_value(value);
        });

        Node::bind(name + "@stick_moved", (FuncInt)[&](const std::string& signal, int dt) {
            set_value(current_value + dt);
            Node::emit_signal(name + "_x", current_value);
        });

        // Text labels (only if slider is enabled)
        {
            float text_scale = 18.0f;

            if (!(parameter_flags & SKIP_NAME)) {
                std::string pretty_name = name;
                to_camel_case(pretty_name);
                text_2d = new Text2D(pretty_name, { 0.f, size.y }, text_scale, TEXT_CENTERED);
                add_child(text_2d);
            }

            if (!disabled && !(parameter_flags & SKIP_VALUE)) {
                std::string value_as_string = value_to_string();
                text_2d_value = new Text2D(value_as_string, { 0.0f, -text_scale }, text_scale, TEXT_CENTERED);
                add_child(text_2d_value);
            }
        }
    }

    void IntSlider2D::update(float delta_time)
    {
        timer += delta_time;

        update_scroll_view();

        if (!visibility)
            return;

        sInputData data = get_input_data();

        if (!disabled && data.is_hovered) {
            IO::push_input(this, data);
        }

        // Update uniforms
        ui_data.hover_info.x = 0.0f;
        ui_data.hover_info.y = 0.0f;
        ui_data.slider_value = static_cast<float>(current_value);

        // Only appear on hover if slider is enabled..
        if (!disabled && text_2d) {
            text_2d->set_visibility(false);
        }

        on_hover = false;

        update_ui_data();

        Node2D::update(delta_time);
    }

    bool IntSlider2D::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        if (text_2d) {
            text_2d->set_visibility(true);
        }

        if (data.is_pressed) {
            float real_size = (mode == HORIZONTAL ? size.x : size.y);
            float local_point = (mode == HORIZONTAL ? data.local_position.x : data.local_position.y);
            // This is at range 0..1
            float normalized_value = glm::clamp(local_point / real_size, 0.f, 1.f) * 100.f;
            // Set in range min-max
            current_value = remap_range(static_cast<int>(normalized_value), 0, 100, min_value, max_value);
            // Make sure it reaches min, max values
            if (current_value < min_value) current_value = min_value;
            else if (current_value > max_value) current_value = max_value;

            if (text_2d_value) {
                text_2d_value->text_entity->set_text(value_to_string());
            }

            // Use curve if needed
            if (parameter_flags & CURVE_INV_POW) {
                current_value = 1.0f / glm::pow(2.0f, current_value);
            }

            Node::emit_signal(name, current_value);
        }

        if (data.was_released) {

            // if enters here, it means it's a double click, so set original value
            if (on_pressed()) {
                set_value(original_value);
                Node::emit_signal(name, current_value);
            }
        }

        if (data.was_hovered) {
            timer = 0.f;
            Engine::instance->vibrate_hand(HAND_RIGHT, HOVER_HAPTIC_AMPLITUDE, HOVER_HAPTIC_DURATION);
        }

        process_wheel_joystick<int>(1, 1);

        on_hover = true;

        hover_factor = glm::cubicEaseOut(glm::clamp(timer / 0.2f, 0.0f, 1.0f));

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.hover_info.y = glm::clamp(hover_factor, 0.0f, 1.0f);
        ui_data.slider_value = static_cast<float>(current_value);

        update_ui_data();

        return true;
    }

    void IntSlider2D::set_value(int new_value)
    {
        current_value = glm::clamp(new_value, min_value, max_value);
        if (text_2d_value) {
            text_2d_value->text_entity->set_text(value_to_string());
        }
    }

    std::string IntSlider2D::value_to_string()
    {
        std::string s = std::to_string(current_value);
        return s.substr(0, s.find('.'));
    }
}
