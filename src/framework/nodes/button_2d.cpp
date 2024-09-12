#include "button_2d.h"

#include <cctype>

#include "engine/engine.h"

#include "framework/input.h"
#include "framework/ui/io.h"
#include "framework/utils/utils.h"
#include "framework/nodes/text.h"
#include "framework/nodes/container_2d.h"
#include "framework/math/math_utils.h"

#include "graphics/renderer.h"
#include "graphics/webgpu_context.h"

#include "glm/gtx/easing.hpp"
#include "glm/gtx/compatibility.hpp"

#include "shaders/mesh_forward.wgsl.gen.h"
#include "shaders/ui/ui_group.wgsl.gen.h"
#include "shaders/ui/ui_button.wgsl.gen.h"
#include "shaders/ui/ui_text_shadow.wgsl.gen.h"

namespace ui {

    /*
    *	Buttons
    */

    Button2D::Button2D(const std::string& sg, const Color& col, uint32_t flags)
        : Button2D(sg, col, flags, { 0.0f, 0.0f }, glm::vec2(BUTTON_SIZE)) { }

    Button2D::Button2D(const std::string& sg, uint32_t flags, const glm::vec2& pos, const glm::vec2& size)
        : Panel2D(sg, pos, size) { }

    Button2D::Button2D(const std::string& sg, const Color& col, uint32_t flags, const glm::vec2& pos, const glm::vec2& size)
        : Panel2D(sg, pos, size, col) {

        class_type = Node2DClassType::BUTTON;

        selected = flags & SELECTED;
        disabled = flags & DISABLED;
        is_unique_selection = flags & UNIQUE_SELECTION;
        allow_toggle = flags & ALLOW_TOGGLE;

        parameter_flags = flags;

        ui_data.is_color_button = is_color_button;
        ui_data.is_button_disabled = disabled;
        ui_data.is_selected = selected;
        ui_data.num_group_items = ComboIndex::UNIQUE;
        ui_data.aspect_ratio = size.x / size.y;

        Material* material = new Material();
        material->set_color(color);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_button::source, shaders::ui_button::path, material));

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);

        // Selection styling visibility callback..
        Node::bind(name + "@pressed", [&](const std::string& signal, void* button) {

            if (disabled)
                return;

            const bool last_value = selected;

            // Unselect siblings
            if (parent && !allow_toggle) {
                for (auto c : parent->get_children()) {
                    Button2D* b = dynamic_cast<Button2D*>(c);
                    if (b) {
                        b->set_selected(false);
                    }
                }
            }

            set_selected(allow_toggle ? !last_value : true);
        });

        set_visibility(!(flags & HIDDEN));
    }

    void Button2D::set_disabled(bool value)
    {
        disabled = value;
        ui_data.is_button_disabled = disabled;
        update_ui_data();

        if (class_type == Node2DClassType::SUBMENU) {

            ButtonSubmenu2D* submenu = static_cast<ButtonSubmenu2D*>(this);
            submenu->box->set_visibility(false);
        }
    }

    void Button2D::set_selected(bool value)
    {
        selected = value;

        // Only unselecting is recursive...
        if (!value)
        {
            for (auto c : children) {

                Button2D* b = dynamic_cast<Button2D*>(c);
                if (b) {
                    b->set_selected(value);
                }
            }
        }
    }

    void Button2D::set_is_unique_selection(bool value)
    {
        is_unique_selection = value;
    }

    void Button2D::update(float delta_time)
    {
        // Set new size

        float current_scale = scaling.x;
        float f = glm::elasticEaseOut(glm::clamp(timer, 0.0f, 1.0f));

        if (current_scale != target_scale) {
            timer += delta_time;
            scaling = glm::vec2(glm::lerp(1.0f, target_scale, f));
        }
        else {
            timer = 0.0f;
        }

        update_scroll_view();

        if (!visibility)
            return;

        sInputData data = get_input_data();

        if(data.is_hovered) {
            IO::push_input(this, data);
        }

        // reset event stuff..
        ui_data.hover_info.x = 0.0f;
        ui_data.hover_info.y = glm::clamp(timer, 0.0f, 1.0f);
        ui_data.is_selected = selected ? 1.f : 0.f;
        ui_data.press_info.x = 0.0f;

        glm::vec2 scale = size * get_scale() * (class_type != Node2DClassType::COMBO_BUTTON ? scaling : glm::vec2(scaling.x, 1.0f));

        ui_data.aspect_ratio = scale.x / scale.y;

        if (text_2d) {
            text_2d->set_visibility(false || label_as_background);
        }

        target_scale = 1.0f;

        on_hover = false;

        update_ui_data();

        Node2D::update(delta_time);
    }

    bool Button2D::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        if (disabled) {
            return true;
        }

        if (text_2d) {
            text_2d->set_visibility(true);
        }

        Panel2D::on_input(data);

        // Internally, use on release mouse, not on press..
        if (data.was_released)
        {
            // close selector if needed
            if (class_type == Node2DClassType::SELECTOR_BUTTON) {

                CircleContainer2D* selector = static_cast<CircleContainer2D*>(get_parent());
                selector->set_visibility(false);
            }

            if (!on_pressed()) {
                // Trigger callback
                Node::emit_signal(name, (void*)this);
                // Visibility stuff..
                Node::emit_signal(name + "@pressed", (void*)nullptr);
            }
        }

        target_scale = 1.1f;

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.hover_info.y = glm::lerp(0.0f, 1.0f, glm::clamp(scaling.x / target_scale, 0.0f, 1.0f));
        ui_data.press_info.x = data.is_pressed ? 1.0f : 0.0f;

        on_hover = true;

        update_ui_data();

        return true;
    }

    void Button2D::set_priority(uint8_t priority)
    {
        Panel2D::set_priority(priority);
    }

    TextureButton2D::TextureButton2D(const std::string& sg, const std::string& texture_path, uint32_t flags)
        : TextureButton2D(sg, texture_path, flags, { 0.0f, 0.0f }) { }

    TextureButton2D::TextureButton2D(const std::string& sg, const std::string& texture_path, uint32_t flags, const glm::vec2& pos, const glm::vec2& size)
        : Button2D(sg, flags, pos, size) {

        class_type = Node2DClassType::TEXTURE_BUTTON;

        is_color_button = false;

        selected = flags & SELECTED;
        disabled = flags & DISABLED;
        is_unique_selection = flags & UNIQUE_SELECTION;
        allow_toggle = flags & ALLOW_TOGGLE;

        parameter_flags = flags;

        ui_data.is_selected = selected;
        ui_data.is_color_button = is_color_button;
        ui_data.is_button_disabled = disabled;
        ui_data.num_group_items = ComboIndex::UNIQUE;
        ui_data.aspect_ratio = size.x / size.y;

        Material* material = new Material();
        material->set_color({ 0.02f, 0.02f, 0.02f, 1.0f });
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);
        material->set_diffuse_texture(texture_path.size() ? RendererStorage::get_texture(texture_path, TEXTURE_STORAGE_UI) : nullptr);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_button::source, shaders::ui_button::path, material));

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);

        // Selection styling visibility callback..
        Node::bind(name + "@pressed", [&](const std::string& signal, void* button) {

            if (disabled || (!is_unique_selection && !allow_toggle))
                return;

            const bool last_value = selected;

            // Unselect siblings
            if (parent && !allow_toggle) {
                for (auto c : parent->get_children()) {
                    Button2D* b = dynamic_cast<Button2D*>(c);
                    if (b) {
                        b->set_selected(false);
                    }
                }
            }
            set_selected(allow_toggle ? !last_value : true);
        });

        // Use label as background
        if (!texture_path.size()) {
            label_as_background = true;
            text_2d = new Text2D(name, {0.0f, size.y * 0.5f - 9.0f}, 18.f, SKIP_TEXT_RECT);
            float w = text_2d->text_entity->get_text_width(name);
            text_2d->translate({ size.x * 0.5f - w * 0.5f, 0.0f });
            add_child(text_2d);
        }

        // Text label
        if (!(flags & SKIP_NAME)) {
            assert(name.size() > 0 && "No signal name size!");

            // Use a prettified text..
            std::string pretty_name = name;
            to_camel_case(pretty_name);
            text_2d = new Text2D(pretty_name, { 0.f, size.y }, 18.f, TEXT_CENTERED);
            text_2d->set_visibility(false);
            add_child(text_2d);
        }

        set_visibility(!(flags& HIDDEN));
    }

    /*
    *   Widget Submenus
    */

    ButtonSubmenu2D::ButtonSubmenu2D(const std::string& sg, const std::string& texture_path, uint32_t flags, const glm::vec2& pos, const glm::vec2& size)
        : TextureButton2D(sg, texture_path, flags, pos, size) {

        class_type = Node2DClassType::SUBMENU;

        box = new ui::HContainer2D("submenu_h_box_" + sg, { 0.0f, 0.0f });
        box->set_visibility(false);
        box->centered = true;

        Node2D::add_child(box);

        // use data with something to force visibility
        Node::bind(sg + "@pressed", [&, box = box](const std::string& sg, void* data) {

            const bool last_value = box->get_visibility();

            for (const auto& w : all_widgets)
            {
                ButtonSubmenu2D* submenu = dynamic_cast<ButtonSubmenu2D*>(w.second);

                if (!submenu)
                    continue;

                bool lower_layer = submenu->get_translation().y > get_translation().y;

                if (lower_layer)
                    continue;

                submenu->box->set_visibility(false);
            }

            box->set_visibility(data ? true : !last_value);
            box->set_position({ (-box->get_size().x + BUTTON_SIZE) * 0.5f, -(box->get_size().y + GROUP_MARGIN)});
        });

        // Submenu icon..
        {
            submenu_mark = new TextureButton2D(sg + "_mark", "data/textures/more.png");
            submenu_mark->set_priority(BUTTON_MARK);
        }
    }

    void ButtonSubmenu2D::render()
    {
        TextureButton2D::render();

        if (submenu_mark) {
            submenu_mark->render();
        }
    }

    void ButtonSubmenu2D::update(float delta_time)
    {
        TextureButton2D::update(delta_time);

        if (submenu_mark) {
            submenu_mark->set_model(get_global_model());
            submenu_mark->scale(glm::vec3(0.6f, 0.6f, 1.0f));
            submenu_mark->translate(glm::vec3(-get_size() * 0.15f, -1e-3f));
        }
    }

    void ButtonSubmenu2D::add_child(Node2D* child)
    {
        box->add_child(child);
    }

    ButtonSelector2D::ButtonSelector2D(const std::string& sg, const std::string& texture_path, uint32_t flags, const glm::vec2& pos, const glm::vec2& size)
        : TextureButton2D(sg, texture_path, flags, pos, size) {

        class_type = Node2DClassType::SELECTOR;

        box = new ui::CircleContainer2D("circle_container", glm::vec2(0.0f, size.y + GROUP_MARGIN));
        box->set_visibility(false);

        Node2D::add_child(box);

        Node::bind(sg, [&, box = box](const std::string& sg, void* data) {

            bool new_value = !box->get_visibility();

            for (const auto& w : all_widgets)
            {
                ButtonSelector2D* selector = dynamic_cast<ButtonSelector2D*>(w.second);

                if (!selector)
                    continue;

                selector->box->set_visibility(false);
            }

            box->set_visibility(new_value);
        });
    }

    void ButtonSelector2D::add_child(Node2D* child)
    {
        Button2D* node_2d = static_cast<Button2D*>(child);
        assert(node_2d);

        node_2d->set_is_unique_selection(true);

        box->add_child(child);
    }

    std::vector<Node*>& ButtonSelector2D::get_children()
    {
        return box->get_children();
    }
}
