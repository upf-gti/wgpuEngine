#include "container_2d.h"

#include <cctype>

#include "engine/engine.h"

#include "framework/input.h"
#include "framework/utils/utils.h"
#include "framework/ui/io.h"
#include "framework/nodes/button_2d.h"

#include "graphics/renderer.h"
#include "graphics/webgpu_context.h"

#include "shaders/mesh_forward.wgsl.gen.h"
#include "shaders/ui/ui_selector.wgsl.gen.h"
#include "shaders/ui/ui_group.wgsl.gen.h"

namespace ui {

    /*
    *	Containers
    */

    Container2D::Container2D(const std::string& name, const glm::vec2& pos, const glm::vec2& s, uint32_t flags, const Color& col)
        : Panel2D(name, pos, s, flags, col)
    {
        class_type = CONTAINER;

        Material* material = new Material();
        material->set_color(color);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_priority(class_type);
        material->set_cull_type(CULL_BACK);
        material->set_depth_read_write(false);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries, material));

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        padding = glm::vec2(2.0f);
        item_margin = glm::vec2(6.0f);

        render_background = false;
    }

    void Container2D::update(float delta_time)
    {
        if (!visibility)
            return;

        Node2D::update(delta_time);

        if (!can_hover)
            return;

        sInputData data = get_input_data();

        if (data.is_hovered) {
            IO::push_input(this, data);
        }
    }

    bool Container2D::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        return true;
    }

    void Container2D::on_children_changed()
    {
        // Recreate quad using new size and reposition accordingly

        if (!quad_mesh->get_surface_count()) {
            return;
        }

        Surface* quad_surface = quad_mesh->get_surface(0);
        quad_surface->create_quad(size.x, size.y, true);

        Node2D::on_children_changed();
    }

    void Container2D::set_hoverable(bool value)
    {
        can_hover = value;
    }

    void Container2D::set_centered(bool value)
    {
        centered = value;
    }

    void Container2D::set_fixed_size(const glm::vec2& new_size)
    {
        use_fixed_size = true;
        fixed_size = new_size;
    }

    HContainer2D::HContainer2D(const std::string& name, const glm::vec2& pos, uint32_t flags, const Color& color)
        : Container2D(name, pos, { 0.0f, 0.0f }, flags, color)
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
            float offset = 0.0f;

            if (node_2d->get_class_type() == TEXT_SHADOW) {
                node_size.x += 16.0f;
                offset = 8.0f;
            }

            node_2d->set_position(padding + glm::vec2(size.x + item_margin.x * static_cast<float>(child_idx) + offset, (size.y - node_size.y) * 0.50f));
            child_idx++;
            size.x += node_size.x;
        }

        size += padding * 2.0f;
        size.x += item_margin.x * static_cast<float>(child_count - 1);

        if (use_fixed_size) {
            size.y = fixed_size.y;
        }

        if (centered) {
            const glm::vec2& pos = get_local_translation();
            // set_position({ (-size.x + BUTTON_SIZE) * 2.0f, pos.y });
            set_position({ (-size.x + BUTTON_SIZE) * 0.5f, -(size.y + GROUP_MARGIN) });
        }

        Container2D::on_children_changed();
    }

    VContainer2D::VContainer2D(const std::string& name, const glm::vec2& pos, uint32_t flags, const Color& color)
        : Container2D(name, pos, { 0.0f, 0.0f }, flags, color)
    {
        class_type = Node2DClassType::VCONTAINER;

        // render_background = false;
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
            node_2d->set_position(glm::vec2(padding.x, size.y + padding.y + item_margin.y * static_cast<float>(child_idx)));
            child_idx++;

            size.x = glm::max(size.x, node_size.x);
            size.y += node_size.y;
        }

        size.x += padding.x * 2.0f;
        size.y = rect_height;

        if (use_fixed_size) {
            size = fixed_size;
        }

        if (centered) {
            const glm::vec2& pos = get_local_translation();
            set_position({ (-size.x + BUTTON_SIZE) * 0.50f, pos.y });
        }

        Container2D::on_children_changed();
    }

    CircleContainer2D::CircleContainer2D(const std::string& name, const glm::vec2& pos, uint32_t flags, const Color& color)
        : Container2D(name, pos, { 0.0f, 0.0f }, flags, color)
    {
        class_type = Node2DClassType::SELECTOR;

        Material* material = new Material();
        material->set_color(color);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_priority(class_type);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_depth_read_write(false);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_selector::source, shaders::ui_selector::path, shaders::ui_selector::libraries, material));

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);

        render_background = true;
    }

    void CircleContainer2D::on_children_changed()
    {
        size_t child_count = get_children().size();
        float radius = BUTTON_SIZE + child_count * 4.0f;

        size = glm::vec2(radius * 2.f + BUTTON_SIZE) * 1.05f;

        float m_pi = glm::pi<float>();

        float cc = (-size.x + BUTTON_SIZE) * 0.5f;
        glm::vec2 center = glm::vec2(cc);

        set_position(center);

        for (size_t i = 0; i < child_count; ++i)
        {
            double angle = -(m_pi / 2.0f) - 2.0f * m_pi * i / (float)child_count;
            glm::vec2 translation = glm::vec2(radius * cos(angle), radius * sin(angle)) - center;

            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            node_2d->set_position(translation);
            node_2d->set_priority(Node2DClassType::SELECTOR_BUTTON);
        }

        Container2D::on_children_changed();
    }

    void CircleContainer2D::update(float delta_time)
    {
        if (!visibility)
            return;

        Container2D::update(delta_time);

        sInputData data = get_input_data();

        glm::vec2 axis_value = Input::get_thumbstick_value(HAND_LEFT);

        // Use this to render triangle marker or not
        ui_data.is_selected = 0.0f;

        if (glm::length(axis_value) > 0.1f)
        {
            ui_data.is_selected = 1.0f;
            ui_data.picker_color.r = fmod(glm::degrees(atan2f(axis_value.y, axis_value.x)), 360.f);

            if (was_input_pressed()) {

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

        if (data.is_hovered) {
            IO::push_input(this, data);
        }
    }

    bool CircleContainer2D::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        // use this to get the event and stop the propagation
        float dist = glm::distance(data.local_position, size * 0.5f);
        // allow using buttons inside the circle..
        return dist > BUTTON_SIZE * 0.5f;
    }

    /*
    *   Widget Group
    */

    ItemGroup2D::ItemGroup2D(const std::string& name, uint32_t flags, const glm::vec2& pos, const Color& color)
        : HContainer2D(name, pos, flags, color) {

        class_type = Node2DClassType::GROUP;

        ui_data.num_group_items = 0;

        Material* material = new Material();
        material->set_color(color);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_priority(class_type);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_depth_read_write(false);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_group::source, shaders::ui_group::path, shaders::ui_group::libraries, material));

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);

        padding = glm::vec2(GROUP_MARGIN);
        item_margin = glm::vec2(GROUP_MARGIN * 0.5f);

        set_visibility(!(parameter_flags & HIDDEN));

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

            if (!node_2d->get_visibility()) {
                child_count--;
                continue;
            }

            if (node_2d->get_class_type() == Node2DClassType::HSLIDER) {
                child_count++;
            }
        }

        set_number_of_items(static_cast<float>(child_count));
    }

    /*
    *   Label
    */

    ImageLabel2D::ImageLabel2D(const std::string& p_text, const std::string& image_path, uint8_t mask, const glm::vec2& scale, float text_scale, const glm::vec2& p)
        : HContainer2D(p_text + "_box", p), mask(mask)
    {
        class_type = Node2DClassType::LABEL;

        padding = glm::vec2(2.0f);
        item_margin = glm::vec2(12.0f, 0.0f);

        Image2D* image = new Image2D(p_text + "_" + image_path, image_path, glm::vec2(32.0f) * scale);
        add_child(image);

        text = new Text2D(p_text, glm::vec2(0.0f), text_scale);
        add_child(text);
    }
}
