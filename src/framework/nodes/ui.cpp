#include "ui.h"

#include <cctype>

#include "framework/math/intersections.h"
#include "framework/input.h"
#include "framework/nodes/text.h"
#include "framework/camera/camera.h"
#include "framework/utils/utils.h"

#include "graphics/renderer.h"
#include "graphics/webgpu_context.h"
#include "spdlog/spdlog.h"
#include "glm/gtx/easing.hpp"
#include "glm/gtx/compatibility.hpp"

#include "shaders/mesh_color.wgsl.gen.h"
#include "shaders/mesh_texture.wgsl.gen.h"
#include "shaders/ui/ui_xr_panel.wgsl.gen.h"
#include "shaders/ui/ui_color_picker.wgsl.gen.h"
#include "shaders/ui/ui_slider.wgsl.gen.h"
#include "shaders/ui/ui_selector.wgsl.gen.h"
#include "shaders/ui/ui_group.wgsl.gen.h"
#include "shaders/ui/ui_button.wgsl.gen.h"
#include "shaders/ui/ui_texture.wgsl.gen.h"
#include "shaders/ui/ui_text_shadow.wgsl.gen.h"

namespace ui {

    /*
    *	Panel
    */

    Node2D* Panel2D::focused = nullptr;

    Panel2D::Panel2D(const std::string& name, const glm::vec2& pos, const glm::vec2& size, const Color& col)
        : Panel2D(name, "", pos, size, col) { }

    Panel2D::Panel2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, const Color& c)
        : Node2D(name, p, s), color(c)
    {
        class_type = Node2DClassType::PANEL;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D;
        material.cull_type = CULL_BACK;
        material.transparency_type = ALPHA_BLEND;
        material.priority = class_type;
        if (image_path.size()) {
            material.diffuse_texture = RendererStorage::get_texture(image_path, true);
            material.shader = RendererStorage::get_shader_from_source(shaders::mesh_texture::source, shaders::mesh_texture::path, material);
        }
        else {
            material.shader = RendererStorage::get_shader_from_source(shaders::mesh_color::source, shaders::mesh_color::path, material);
        }

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y);

        quad_mesh.add_surface(quad_surface);
        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);
    }

    void Panel2D::set_color(const Color& c)
    {
        color = c;

        quad_mesh.set_surface_material_override_color(0, c);
    }

    void Panel2D::render()
    {
        if (!visibility)
            return;

        if (class_type == PANEL) {
            int a = 1;
        }

        if (render_background) {
            // Convert the mat3x3 to mat4x4
            uint8_t priority = class_type;
            glm::vec2 position = get_translation() + size * 0.5f * get_scale();
            // Don't apply the scaling to combo buttons.. 
            glm::vec2 scale = get_scale() * (class_type != Node2DClassType::COMBO_BUTTON ? scaling : glm::vec2(scaling.x, 1.0f));
            glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(position, -priority * 1e-4));
            model = glm::scale(model, glm::vec3(scale, 1.0f));
            model = get_global_viewport_model() * model;
            Renderer::instance->add_renderable(&quad_mesh, model);
        }

        Node2D::render();
    }

    sInputData Panel2D::get_input_data()
    {
        sInputData data;

        WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

        Material* material = quad_mesh.get_surface_material_override(quad_mesh.get_surface(0));

        if (material->flags & MATERIAL_2D)
        {
            glm::vec2 mouse_pos = Input::get_mouse_position();
            mouse_pos.y = webgpu_context->render_height - mouse_pos.y;

            glm::vec2 min = get_translation();
            glm::vec2 max = min + size;

            data.is_hovered = mouse_pos.x >= min.x && mouse_pos.y >= min.y && mouse_pos.x <= max.x && mouse_pos.y <= max.y;
            data.was_pressed = data.is_hovered && Input::was_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
            if (data.was_pressed) {
                pressed_inside = true;
            }
            data.was_released = Input::was_mouse_released(GLFW_MOUSE_BUTTON_LEFT);
            if (data.was_released) {
                pressed_inside = false;
            }
            data.is_pressed = pressed_inside && Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);

            glm::vec2 local_mouse_pos = mouse_pos - get_translation();
            data.local_position = glm::vec2(local_mouse_pos.x, size.y - local_mouse_pos.y);
        }
        else {

            glm::vec3 ray_origin;
            glm::vec3 ray_direction;

            // Handle ray using VR controller
            if (Renderer::instance->get_openxr_available())
            {
                // Ray
                ray_origin = Input::get_controller_position(HAND_RIGHT, POSE_AIM);
                glm::mat4x4 select_hand_pose = Input::get_controller_pose(HAND_RIGHT, POSE_AIM);
                ray_direction = get_front(select_hand_pose);
            }
            // Handle ray using mouse position
            else
            {
                Camera* camera = Renderer::instance->get_camera();
                glm::vec3 ray_dir = camera->screen_to_ray(Input::get_mouse_position());

                // Ray
                ray_origin = camera->get_eye();
                ray_direction = glm::normalize(ray_dir);
            }

            // Quad
            uint8_t priority = class_type;
            glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(get_translation(), -priority * 1e-4));
            model = get_global_viewport_model() * model;

            glm::vec3 quad_position = model[3];
            glm::quat quad_rotation = glm::quat_cast(model);
            glm::vec2 quad_size = size * get_scale();

            float collision_dist;
            glm::vec3 intersection_point;
            glm::vec3 local_intersection_point;

            data.is_hovered = intersection::ray_quad(
                ray_origin,
                ray_direction,
                quad_position,
                quad_size,
                quad_rotation,
                intersection_point,
                local_intersection_point,
                collision_dist,
                false
            );

            if (Renderer::instance->get_openxr_available()) {
                data.was_pressed = data.is_hovered && Input::was_button_pressed(XR_BUTTON_A);
                if (data.was_pressed) {
                    pressed_inside = true;
                }
                data.was_released = Input::was_button_released(XR_BUTTON_A);
                if (data.was_released) {
                    pressed_inside = false;
                }
                data.is_pressed = pressed_inside && Input::is_button_pressed(XR_BUTTON_A);
            }
            else {
                data.was_pressed = data.is_hovered && Input::was_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
                if (data.was_pressed) {
                    pressed_inside = true;
                }
                data.was_released = Input::was_mouse_released(GLFW_MOUSE_BUTTON_LEFT);
                if (data.was_released) {
                    pressed_inside = false;
                }
                data.is_pressed = pressed_inside && Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
            }

            data.ray_distance = collision_dist;

            glm::vec2 local_pos = glm::vec2(local_intersection_point) / get_scale();
            data.local_position = glm::vec2(local_pos.x, size.y - local_pos.y);
        }

        if (!on_hover && data.is_hovered) {
            data.was_hovered = true;
        }

        // Few logic for managing focus

        if (data.was_pressed) {
            focused = this;
        }

        data.is_pressed &= (focused == this);

        return data;
    }

    void Panel2D::remove_flag(uint8_t flag)
    {
        Material* material = quad_mesh.get_surface_material_override(quad_mesh.get_surface(0));
        material->flags ^= flag;

        Node2D::remove_flag(flag);
    }

    void Panel2D::set_priority(uint8_t priority)
    {
        Material* material = quad_mesh.get_surface_material_override(quad_mesh.get_surface(0));
        material->priority = priority;

        Node2D::set_priority(priority);
    }

    void Panel2D::update_ui_data()
    {
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, &quad_mesh, ui_data);
    }

    /*
    *   Image
    */

    XRPanel::XRPanel(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s)
        : Panel2D(name, image_path, p, s, colors::WHITE)
    {
        Surface* quad_surface = quad_mesh.get_surface(0);
        quad_surface->create_subvidided_quad(size.x, size.y);

        Material* material = quad_mesh.get_surface_material_override(quad_surface);
        material->flags |= MATERIAL_UI;
        // material->shader = RendererStorage::get_shader_from_source(shaders::ui_xr_panel::source, shaders::ui_xr_panel::path, *material);
        material->shader = RendererStorage::get_shader("data/shaders/ui_xr_panel.wgsl", *material);

        ui_data.xr_info = glm::vec4(1.0f, 1.0f, 0.5f, 0.5f);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->shader, &quad_mesh, ui_data, 3);
    }

    void XRPanel::update(float delta_time)
    {
        Node2D::update(delta_time);

        if (!visibility)
            return;

        sInputData data = get_input_data();

        quad_mesh.set_surface_material_override_color(0, data.is_hovered ? colors::RED : colors::WHITE);

        ui_data.aspect_ratio = size.x / size.y;

        update_ui_data();
    }

    void XRPanel::add_button(const glm::vec2& p, const glm::vec2& s, const Color& c)
    {
        XRPanel* new_button = new XRPanel("xr_button", "data/textures/menu_buttons/skip.png", { 0.0f, 0.0f }, size);
        add_child(new_button);

        new_button->set_priority(PANEL - 1u);

        new_button->ui_data.xr_info = glm::clamp(glm::vec4(s / size, p / size), 0.0f, 1.0f);
        new_button->ui_data.picker_color = c;

        new_button->update_ui_data();
    }

    /*
    *	Containers
    */

    Container2D::Container2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Panel2D(name, pos, { 0.0f, 0.0f }, col)
    {
        Material material;
        material.color = color;
        material.flags = MATERIAL_2D;
        material.priority = class_type;
        material.cull_type = CULL_BACK;
        material.shader = RendererStorage::get_shader_from_source(shaders::mesh_color::source, shaders::mesh_color::path, material);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        padding = glm::vec2(2.0f);
        item_margin = glm::vec2(6.0f);

        render_background = false;
    }

    void Container2D::on_children_changed()
    {
        // Recreate quad using new size and reposition accordingly

        if (!quad_mesh.get_surface_count()) {
            return;
        }

        Surface* quad_surface = quad_mesh.get_surface(0);
        quad_surface->create_quad(size.x, size.y);

        Node2D::on_children_changed();
    }

    void Container2D::set_centered(bool value)
    {
        centered = value;
    }

    HContainer2D::HContainer2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Container2D(name, pos, col)
    {
        class_type = Node2DClassType::HCONTAINER;
    }

    void HContainer2D::on_children_changed()
    {
        size = glm::vec2(0.0f);

        size_t child_count = get_children().size();

        // Get HEIGHT in first pass..
        for (size_t i = 0; i < child_count; ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            if (!node_2d->get_visibility()) {
                continue;
            }
            size.y = glm::max(size.y, node_2d->get_size().y);
        }

        // Reorder in WIDTH..
        size_t child_idx = 0;
        for (size_t i = 0; i < get_children().size(); ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            if (!node_2d->get_visibility()) {
                child_count--;
                continue;
            }
            glm::vec2 node_size = node_2d->get_size();
            node_2d->set_translation(padding + glm::vec2(size.x + item_margin.x * static_cast<float>(child_idx), (size.y - node_size.y) * 0.50f));
            child_idx++;
            size.x += node_size.x;
        }

        size += padding * 2.0f;
        size.x += item_margin.x * static_cast<float>(child_count - 1);

        if (centered) {
            const glm::vec2& pos = get_local_translation();
            set_translation({ (-size.x + BUTTON_SIZE) * 0.50f, pos.y });
        }

        Container2D::on_children_changed();
    }

    VContainer2D::VContainer2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Container2D(name, pos, col)
    {
        class_type = Node2DClassType::VCONTAINER;
    }

    void VContainer2D::on_children_changed()
    {
        size_t child_count = get_children().size();
        float rect_height = 0.0f;
        size = glm::vec2(0.0f);

        // Get HEIGHT in first pass..
        for (size_t i = 0; i < get_children().size(); ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            if (!node_2d->get_visibility()) {
                child_count--;
                continue;
            }
            rect_height += node_2d->get_size().y;
        }

        rect_height += padding.y * 2.0f + item_margin.y * static_cast<float>(child_count - 1);

        size_t child_idx = 0;
        for (size_t i = 0; i < get_children().size(); ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            if (!node_2d->get_visibility()) {
                continue;
            }
            glm::vec2 node_size = node_2d->get_size();
            node_2d->set_translation(glm::vec2(padding.x, rect_height - (size.y + node_size.y) - padding.y - item_margin.y * static_cast<float>(child_idx)));
            child_idx++;

            size.x = glm::max(size.x, node_size.x);
            size.y += node_size.y;
        }

        size.x += padding.x * 2.0f;
        size.y = rect_height;

        if (centered) {
            const glm::vec2& pos = get_local_translation();
            set_translation({ (-size.x + BUTTON_SIZE) * 0.50f, pos.y });
        }

        Container2D::on_children_changed();
    }

    CircleContainer2D::CircleContainer2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Container2D(name, pos, col)
    {
        class_type = Node2DClassType::SELECTOR;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.priority = class_type;
        material.transparency_type = ALPHA_BLEND;
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_selector::source, shaders::ui_selector::path, material);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        render_background = true;
    }

    void CircleContainer2D::on_children_changed()
    {
        size_t child_count = get_children().size();
        float radius = BUTTON_SIZE + child_count * 3.0f;

        size = glm::vec2(radius * 2.f + BUTTON_SIZE) * 1.05f;

        constexpr float m_pi = glm::pi<float>();

        float cc = (-size.x + BUTTON_SIZE) * 0.5f;
        glm::vec2 center = glm::vec2(cc);

        set_translation(center);

        for (size_t i = 0; i < child_count; ++i)
        {
            double angle = (m_pi / 2.0f) + 2.0f * m_pi * i / (float)child_count;
            glm::vec2 translation = glm::vec2(radius * cos(angle), radius * sin(angle)) - center;

            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            node_2d->set_translation(translation);
            node_2d->set_priority(Node2DClassType::SELECTOR_BUTTON);
        }

        Container2D::on_children_changed();
    }

    void CircleContainer2D::update(float delta_time)
    {
        Container2D::update(delta_time);

        if (!visibility)
            return;

        sInputData data = get_input_data();

        glm::vec2 axis_value = Input::get_thumbstick_value(HAND_LEFT);

        // Use this to render triangle marker or not
        ui_data.is_selected = 0.0f;

        if (glm::length(axis_value) > 0.1f)
        {
            ui_data.is_selected = 1.0f;
            ui_data.picker_color.r = fmod(glm::degrees(atan2f(axis_value.y, axis_value.x)), 360.f);

            if (Input::was_button_pressed(XR_BUTTON_A)) {

                auto& childs = get_children();
                size_t child_count = childs.size();
                float angle = ui_data.picker_color.r - 65.f;
                if (angle < 0.0f) angle += 360.f;
                size_t index = angle / 360.f * child_count;
                assert(index >= 0 && index < child_count);

                Node2D* element = static_cast<Node2D*>(childs[index]);

                sInputData new_data;
                new_data.was_pressed = true;
                element->on_input(new_data);
            }
        }

        update_ui_data();

        if (data.is_hovered)
        {
            Node2D::push_input(this, data);
        }
    }

    bool CircleContainer2D::on_input(sInputData data)
    {
        // use this to get the event and stop the propagation
        float dist = glm::distance(data.local_position, size * 0.5f);
        // allow using buttons inside the circle..
        return dist > BUTTON_SIZE * 0.5f;
    }

    /*
    *   Text
    */


    Text2D::Text2D(const std::string& _text, float scale, bool center_big)
        : Text2D(_text, { 0.0f, 0.0f }, scale, colors::WHITE)
    {
        glm::vec2 centered_position = { -size.x * 0.5f + BUTTON_SIZE * (center_big ? 1.0f : 0.5f), -size.y * 2.f };
        set_translation(centered_position);
    }

    Text2D::Text2D(const std::string& _text, const glm::vec2& pos, float scale, const Color& color, bool center_big)
        : Panel2D(_text + "@text", pos, {1.0f, 1.0f}) {

        text_string = _text;
        text_scale = scale;

        class_type = Node2DClassType::TEXT_SHADOW;

        text_entity = new TextEntity(text_string);
        text_entity->set_scale(text_scale);
        text_entity->generate_mesh(color, MATERIAL_2D);
        text_entity->set_surface_material_priority(0, Node2DClassType::TEXT);

        float text_width = (float)text_entity->get_text_width(text_string);
        size.x = std::max(text_width, 24.0f);
        size.y = text_scale * 0.5f;

        ui_data.num_group_items = size.x;

        Material material;
        material.color = colors::RED;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.transparency_type = ALPHA_BLEND;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_text_shadow::source, shaders::ui_text_shadow::path, material);

        Surface* quad_surface = quad_mesh.get_surface(0);
        quad_surface->create_quad(this->size.x + TEXT_SHADOW_MARGIN * text_scale, this->size.y + TEXT_SHADOW_MARGIN * text_scale);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        glm::vec2 centered_position = { -size.x * 0.5f + BUTTON_SIZE * (center_big ? 1.0f : 0.5f), pos.y };
        set_translation(centered_position);
    }

    void Text2D::update(float delta_time)
    {
        if (!visibility)
            return;

        // Convert the mat3x3 to mat4x4
        uint8_t priority = class_type;
        glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(get_translation(), -priority * 1e-4));
        model = glm::scale(model, glm::vec3(get_scale(), 1.0f));
        model = get_global_viewport_model() * model;

        text_entity->set_model(model);

        Panel2D::update(delta_time);
    }

    void Text2D::render()
    {
        if (!visibility)
            return;

        Panel2D::render();

        text_entity->render();
    }

    void Text2D::remove_flag(uint8_t flag)
    {
        uint8_t flags = text_entity->get_flags();
        flags ^= MATERIAL_2D;

        text_entity->generate_mesh(color, (eMaterialFlags)flags);

        Panel2D::remove_flag(flag);
    }

    void Text2D::set_priority(uint8_t priority)
    {
        text_entity->set_surface_material_priority(0, priority);

        Panel2D::set_priority(priority);
    }

    /*
    *	Texture (Image)
    */

    Texture2D::Texture2D(const std::string& name, const std::string& texture_path, const glm::vec2& size, const glm::vec2& pos)
        : Panel2D(name, pos, size, colors::WHITE)
    {
        class_type = Node2DClassType::TEXTURE;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.transparency_type = ALPHA_BLEND;
        material.priority = class_type;
        material.diffuse_texture = RendererStorage::get_texture(texture_path, true);
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_texture::source, shaders::ui_texture::path, material);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);
    }

    /*
    *	Buttons
    */

    Button2D::Button2D(const std::string& sg, const Color& col, uint8_t flags)
        : Button2D(sg, col, flags, { 0.0f, 0.0f }, glm::vec2(BUTTON_SIZE)) { }

    Button2D::Button2D(const std::string& sg, uint8_t flags, const glm::vec2& pos, const glm::vec2& size)
        : Panel2D(sg, pos, size), signal(sg) { }

    Button2D::Button2D(const std::string& sg, const Color& col, uint8_t flags, const glm::vec2& pos, const glm::vec2& size)
        : Panel2D(sg, pos, size, col), signal(sg) {

        class_type = Node2DClassType::BUTTON;

        selected = flags & SELECTED;
        disabled = flags & DISABLED;
        is_unique_selection = flags & UNIQUE_SELECTION;
        allow_toggle = flags & ALLOW_TOGGLE;
        keep_rgb = flags & KEEP_RGB;

        parameter_flags = flags;

        ui_data.keep_rgb = keep_rgb;
        ui_data.is_color_button = is_color_button;
        ui_data.is_button_disabled = disabled;
        ui_data.is_selected = selected;
        ui_data.num_group_items = ComboIndex::UNIQUE;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.transparency_type = ALPHA_BLEND;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_button::source, shaders::ui_button::path, material);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        // Selection styling visibility callback..
        Node::bind(signal + "@pressed", [&](const std::string& signal, void* button) {

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
    }

    void Button2D::on_pressed()
    {
        // on press, close button selector..

        if (class_type == Node2DClassType::SELECTOR_BUTTON) {

            CircleContainer2D* selector = static_cast<CircleContainer2D*>(get_parent());
            selector->set_visibility(false);
        }
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

        Panel2D::update(delta_time);

        if (!visibility || ui_data.is_button_disabled)
            return;

        sInputData data = get_input_data();

        if(data.is_hovered)
        {
            Node2D::push_input(this, data);
        }

        // reset event stuff..
        ui_data.hover_info.x = 0.0f;
        ui_data.hover_info.y = glm::clamp(timer, 0.0f, 1.0f);
        ui_data.is_selected = selected ? 1.f : 0.f;

        glm::vec2 scale = get_scale() * (class_type != Node2DClassType::COMBO_BUTTON ? scaling : glm::vec2(scaling.x, 1.0f));
        ui_data.aspect_ratio = scale.x / scale.y;

        if (text_2d) {
            text_2d->set_visibility(false);
        }

        target_scale = 1.0f;

        on_hover = false;

        update_ui_data();
    }

    bool Button2D::on_input(sInputData data)
    {
        if (text_2d) {
            text_2d->set_visibility(true);
        }

        if (data.was_pressed)
        {
            // Trigger callback
            Node::emit_signal(signal, (void*)this);
            // Visibility stuff..
            Node::emit_signal(signal + "@pressed", (void*)nullptr);

            on_pressed();
        }

        target_scale = 1.1f;

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.hover_info.y = glm::lerp(0.0f, 1.0f, glm::clamp(scaling.x / target_scale, 0.0f, 1.0f));

        on_hover = true;

        update_ui_data();

        return true;
    }

    void Button2D::set_priority(uint8_t priority)
    {
        Panel2D::set_priority(priority);
    }

    TextureButton2D::TextureButton2D(const std::string& sg, const std::string& texture_path, uint8_t flags)
        : TextureButton2D(sg, texture_path, flags, { 0.0f, 0.0f }) { }

    TextureButton2D::TextureButton2D(const std::string& sg, const std::string& texture_path, uint8_t flags, const glm::vec2& pos, const glm::vec2& size)
        : Button2D(sg, flags, pos, size) {

        class_type = Node2DClassType::TEXTURE_BUTTON;

        is_color_button = false;

        selected = flags & SELECTED;
        disabled = flags & DISABLED;
        is_unique_selection = flags & UNIQUE_SELECTION;
        allow_toggle = flags & ALLOW_TOGGLE;
        keep_rgb = flags & KEEP_RGB;

        parameter_flags = flags;

        ui_data.is_selected = selected;
        ui_data.keep_rgb = keep_rgb;
        ui_data.is_color_button = is_color_button;
        ui_data.is_button_disabled = disabled;
        ui_data.num_group_items = ComboIndex::UNIQUE;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.transparency_type = ALPHA_BLEND;
        material.priority = class_type;
        material.diffuse_texture = RendererStorage::get_texture(texture_path, true);
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_button::source, shaders::ui_button::path, material);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        // Selection styling visibility callback..
        Node::bind(signal + "@pressed", [&](const std::string& signal, void* button) {

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

        // Text label
        {
            assert(signal.size() > 0 && "Please use signals with any letter..");

            // Use a prettified text..
            std::string pretty_name = signal;
            to_camel_case(pretty_name);
            text_2d = new Text2D(pretty_name, 18.f);
            text_2d->set_visibility(false);
            add_child(text_2d);
        }
    }

    /*
    *   Widget Group
    */

    ItemGroup2D::ItemGroup2D(const std::string& name, const glm::vec2& pos, const Color& color)
        : HContainer2D(name, pos, color) {

        class_type = Node2DClassType::GROUP;

        ui_data.num_group_items = 0;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.priority = class_type;
        material.transparency_type = ALPHA_BLEND;
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_group::source, shaders::ui_group::path, material);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        padding = glm::vec2(GROUP_MARGIN);
        item_margin = glm::vec2(GROUP_MARGIN * 0.5f);

        render_background = true;
    }

    float ItemGroup2D::get_number_of_items()
    {
        return ui_data.num_group_items;
    }

    void ItemGroup2D::set_number_of_items(float number)
    {
        ui_data.num_group_items = number;
        ui_data.aspect_ratio = number;

        update_ui_data();
    }

    void ItemGroup2D::on_children_changed()
    {
        HContainer2D::on_children_changed();

        size_t child_count = get_children().size();

        for (size_t i = 0; i < get_children().size(); ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);

            if (node_2d->get_class_type() != Node2DClassType::SLIDER) {
                continue;
            }

            Slider2D* slider = static_cast<Slider2D*>(node_2d);
            if (slider->mode == HORIZONTAL) {
                child_count++;
            }
        }

        set_number_of_items(static_cast<float>(child_count));
    }

    /*
    *   Combo buttons
    */

    ComboButtons2D::ComboButtons2D(const std::string& name, const glm::vec2& pos, const Color& color)
        : HContainer2D(name, pos, color) {

        class_type = Node2DClassType::COMBO;

        item_margin = glm::vec2(-6.0f);
    }

    void ComboButtons2D::on_children_changed()
    {
        size_t child_count = get_children().size();

        for (size_t i = 0; i < child_count; ++i)
        {
            Button2D* node_2d = static_cast<Button2D*>(get_children()[i]);

            if (child_count == 1) {
                node_2d->ui_data.num_group_items = ComboIndex::UNIQUE;
            }
            else if (child_count == 2) {
                node_2d->ui_data.num_group_items = float(i == 0 ? ComboIndex::FIRST : ComboIndex::LAST);
            }
            else {
                node_2d->ui_data.num_group_items = float(i == 0 ? ComboIndex::FIRST : (i == child_count - 1 ? ComboIndex::LAST : ComboIndex::MIDDLE));
            }

            node_2d->set_priority(Node2DClassType::COMBO_BUTTON);
            node_2d->set_is_unique_selection(true);
            node_2d->update_ui_data();
        }

        HContainer2D::on_children_changed();
    }

    /*
    *   Widget Submenus
    */

    ButtonSubmenu2D::ButtonSubmenu2D(const std::string& sg, const std::string& texture_path, uint8_t flags, const glm::vec2& pos, const glm::vec2& size)
        : TextureButton2D(sg, texture_path, flags, pos, size) {

        class_type = Node2DClassType::SUBMENU;

        box = new ui::HContainer2D("submenu_h_box_" + sg, glm::vec2(0.0f, size.y + GROUP_MARGIN));
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

                bool lower_layer = submenu->get_translation().y < get_translation().y;

                if (lower_layer)
                    continue;

                submenu->box->set_visibility(false);
            }

            box->set_visibility(data ? true : !last_value);
        });

        // Submenu icon..
        {
            submenu_mark = new TextureButton2D(sg + "@mark", "data/textures/more.png");
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

        if (submenu_mark)
        {
            submenu_mark->set_model(get_global_model());
            submenu_mark->scale(glm::vec3(0.6f, 0.6f, 1.0f));
            submenu_mark->translate(glm::vec3(-get_size().x * 0.15f, get_size().y * 0.85f, -1e-3f));
        }
    }

    void ButtonSubmenu2D::add_child(Node2D* child)
    {
        box->add_child(child);
    }

    ButtonSelector2D::ButtonSelector2D(const std::string& sg, const std::string& texture_path, uint8_t flags, const glm::vec2& pos, const glm::vec2& size)
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

    /*
    *	Slider
    */

    Slider2D::Slider2D(const std::string& sg, float v, int mode, uint8_t flags, float min, float max, int precision)
        : Slider2D(sg, "", v, {0.0f, 0.0f}, glm::vec2(BUTTON_SIZE), mode, flags, min, max, precision) {}

    Slider2D::Slider2D(const std::string& sg, const std::string& texture_path, float v, int mode, uint8_t flags, float min, float max, int precision)
        : Slider2D(sg, texture_path, v, { 0.0f, 0.0f }, glm::vec2(BUTTON_SIZE), mode, flags, min, max, precision) {}

    Slider2D::Slider2D(const std::string& sg, const std::string& texture_path, float value, const glm::vec2& pos, const glm::vec2& size, int mode, uint8_t flags, float min, float max, int precision)
        : Panel2D(sg, pos, size), signal(sg), current_value(value), min_value(min), max_value(max), precision(precision) {

        this->class_type = Node2DClassType::SLIDER;
        this->mode = mode;

        disabled = flags & DISABLED;

        bool is_horizontal = (mode == SliderMode::HORIZONTAL);
        ui_data.num_group_items = is_horizontal ? 2.f : 1.f;
        ui_data.slider_max = max_value;
        ui_data.slider_min = min_value;
        ui_data.is_button_disabled = disabled;

        parameter_flags = flags;

        this->size = glm::vec2(size.x * ui_data.num_group_items, size.y);

        current_value = glm::clamp(current_value, min_value, max_value);

        Material material;
        material.color = colors::WHITE;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.transparency_type = ALPHA_BLEND;
        material.priority = class_type;
        material.diffuse_texture = texture_path.size() > 0 ? RendererStorage::get_texture(texture_path, true) : nullptr;
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_slider::source, shaders::ui_slider::path, material);

        Surface* quad_surface = quad_mesh.get_surface(0);
        quad_surface->create_quad(this->size.x, this->size.y);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        Node::bind(signal + "@changed", [&](const std::string& signal, float value) {
            set_value(value);
            });

        // Text labels (only if slider is enabled)
        {
            std::string pretty_name = signal;
            to_camel_case(pretty_name);
            text_2d = new Text2D(pretty_name, 18.f, is_horizontal);
            add_child(text_2d);

            if (!disabled && !(flags & SKIP_VALUE)) {
                std::string value_as_string = value_to_string();
                text_2d_value = new Text2D(value_as_string, {0.0f, size.y * 1.2f}, 17.f, colors::WHITE, is_horizontal);
                add_child(text_2d_value);
            }
        }
    }

    void Slider2D::render()
    {
        Panel2D::render();
    }

    void Slider2D::update(float delta_time)
    {
        Panel2D::update(delta_time);

        timer += delta_time;

        if (!visibility)
            return;

        sInputData data = get_input_data();

        if (!disabled && data.is_hovered)
        {
            Node2D::push_input(this, data);
        }

        // Update uniforms
        ui_data.hover_info.x = 0.0f;
        ui_data.hover_info.y = 0.0f;
        ui_data.slider_value = current_value;
        ui_data.aspect_ratio = (mode == HORIZONTAL ? 2.0f : 1.0f);

        // Only appear on hover if slider is enabled..
        if (!disabled) {
            text_2d->set_visibility(false);
        }

        on_hover = false;

        update_ui_data();
    }

    bool Slider2D::on_input(sInputData data)
    {
        text_2d->set_visibility(true);

        if (data.is_pressed)
        {
            float real_size = (mode == HORIZONTAL ? size.x : size.y);
            float local_point = (mode == HORIZONTAL ? data.local_position.x : size.y - data.local_position.y);
            // this is at range 0..1
            current_value = glm::clamp(local_point / real_size, 0.f, 1.f);
            // set in range min-max
            current_value = remap_range(current_value, 0.0f, 1.0f, min_value, max_value);
            // make sure it reaches min, max values
            if (fabsf(current_value - min_value) < 1e-4f) current_value = min_value;
            else if (fabsf(current_value - max_value) < 1e-4f) current_value = max_value;
            // emit signal to use new value
            Node::emit_signal(signal, current_value);

            if (text_2d_value) {
                text_2d_value->text_entity->set_text(value_to_string());
            }

            on_pressed();
        }

        if (data.was_hovered) {
            timer = 0.f;
        }

        on_hover = true;

        hover_factor = glm::cubicEaseOut(glm::clamp(timer / 0.2f, 0.0f, 1.0f));

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.hover_info.y = glm::clamp(hover_factor, 0.0f, 1.0f);

        update_ui_data();

        return true;
    }

    void Slider2D::set_value(float new_value)
    {
        current_value = glm::clamp(new_value, min_value, max_value);
        if (text_2d_value) {
            text_2d_value->text_entity->set_text(value_to_string());
        }
    }

    void Slider2D::set_disabled(bool new_disabled)
    {
        disabled = new_disabled;
        ui_data.is_button_disabled = disabled;
        update_ui_data();
    }

    std::string Slider2D::value_to_string()
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
    *	ColorPicker
    */

    ColorPicker2D::ColorPicker2D(const std::string& sg, const Color& c)
        : ColorPicker2D(sg, { 0.0f, 0.0f }, glm::vec2(PICKER_SIZE), c) {}

    ColorPicker2D::ColorPicker2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size, const Color& c)
        : Panel2D(sg, pos, size, c), signal(sg)
    {
        class_type = Node2DClassType::COLOR_PICKER;

        Material material;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.cull_type = CULL_BACK;
        material.transparency_type = ALPHA_BLEND;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader_from_source(shaders::ui_color_picker::source, shaders::ui_color_picker::path, material);

        color = { 0.0f, 1.0f, 1.0f, 1.0f };

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        Node::bind(signal + "@changed", [&](const std::string& signal, Color color) {

            glm::vec3 new_color = rgb2hsv(glm::pow(glm::vec3(color), glm::vec3(1.0f / 2.2f)));

            this->color = glm::vec4(new_color, 1.0f);
        });
    }

    void ColorPicker2D::update(float delta_time)
    {
        Panel2D::update(delta_time);

        if (!visibility)
            return;

        sInputData data = get_input_data();

        bool hovered = data.is_hovered;

        glm::vec2 local_mouse_pos = data.local_position;
        local_mouse_pos /= size;
        local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1
        float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

        // Update hover
        hovered &= (dist < 1.f);

        if (hovered) {
            Node2D::push_input(this, data);
        }
        else {
            changing_hue = changing_sv = false;
        }

        // Update uniforms
        ui_data.hover_info.x = 0.0f;
        ui_data.picker_color = color;

        update_ui_data();
    }

    bool ColorPicker2D::on_input(sInputData data)
    {
        glm::vec2 local_mouse_pos = data.local_position;
        local_mouse_pos /= size;
        local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1

        float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

       if (data.is_pressed)
       {
           // Change HUE
           if ((changing_hue && dist > 0.1f) || (dist > (1.0f - ring_thickness) && !changing_sv)) {
               color.r = fmod(glm::degrees(atan2f(-local_mouse_pos.y, local_mouse_pos.x)), 360.f);
               changing_hue = true;
           }

           // Update Saturation, Value
           else if(!changing_hue){
               glm::vec2 uv = local_mouse_pos;
               uv.y *= -1.0f;
               glm::vec2 sv = uv_to_saturation_value(uv);
               color = { color.r, sv.x, sv.y, 1.0f };
               changing_sv = true;
           }

           // Send the signal using the final color
           glm::vec3 new_color = glm::pow(hsv2rgb(color), glm::vec3(2.2f));
           Node::emit_signal(signal, glm::vec4(new_color, 1.0));

           on_pressed();
       }

       if (data.was_released)
       {
           Node::emit_signal(signal + "@released", color);
           changing_hue = changing_sv = false;
       }

        // Update uniforms
        ui_data.hover_info.x = 1.0f;

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

    /*
    *   Label
    */

    ImageLabel2D::ImageLabel2D(const std::string& p_text, const std::string& image_path, uint8_t mask, const glm::vec2& scale, float text_scale, const glm::vec2& p)
        : HContainer2D(p_text + "@box", p), mask(mask) {

        class_type = Node2DClassType::LABEL;

        padding = glm::vec2(2.0f);
        item_margin = glm::vec2(12.0f, 0.0f);

        Texture2D* image = new Texture2D(p_text + "_" + image_path, image_path, glm::vec2(32.0f) * scale);
        add_child(image);

        text = new Text2D(p_text, glm::vec2(0.0f), text_scale);
        add_child(text);
    }
}
