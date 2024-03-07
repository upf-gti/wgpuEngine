#include "ui.h"
#include "framework/utils/utils.h"
#include "framework/ui/ui_controller.h"
#include "framework/utils/intersections.h"
#include "framework/input.h"
#include "framework/nodes/text.h"
#include "graphics/renderer.h"
#include "graphics/webgpu_context.h"

#include "spdlog/spdlog.h"

namespace ui {

    /*
    *	Panel
    */

    Panel2D::Panel2D(const std::string& name, const glm::vec2& pos, const glm::vec2& size, const Color& col)
        : Node2D(name, pos, size), color(col)
    {
        type = Node2DType::PANEL;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D;
        material.priority = type;
        material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y);

        quad = new MeshInstance3D();
        quad->add_surface(quad_surface);
        quad->set_surface_material_override(quad->get_surface(0), material);
    }

    void Panel2D::set_color(const Color& c)
    {
        color = c;

        quad->set_surface_material_override_color(0, c);
    }

    void Panel2D::update(float delta_time)
    {
        if (!visibility)
            return;

        // Move quad to node position..

        glm::vec2 t = get_translation() + size * 0.50f;

        if (centered && parent) {
            auto psize = parent->get_size();
            t.x = get_translation().x + psize.x * 0.5f;
        }

        quad->set_translation(glm::vec3(t, 0.0f));

        Node2D::update(delta_time);
    }

    void Panel2D::render()
    {
        if (!visibility)
            return;

        quad->render();

        Node2D::render();
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
        material.priority = type;
        material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y);

        quad = new MeshInstance3D();
        quad->add_surface(quad_surface);
        quad->set_surface_material_override(quad->get_surface(0), material);

        padding = glm::vec2(GROUP_MARGIN);
        item_margin = glm::vec2(GROUP_MARGIN);
    }

    void Container2D::on_children_changed()
    {
        // Recreate quad using new size and reposition accordingly

        Surface* quad_surface = quad->get_surface(0);
        quad_surface->create_quad(size.x, size.y);

        Node2D::on_children_changed();
    }

    HContainer2D::HContainer2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Container2D(name, pos, col)
    {
        type = Node2DType::HCONTAINER;
    }

    void HContainer2D::on_children_changed()
    {
        size_t child_count = get_children().size();
        size = glm::vec2(0.0f);

        for (size_t i = 0; i < child_count; ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            node_2d->set_translation(padding + glm::vec2(size.x + item_margin.x * static_cast<float>(i), 0.0f));

            glm::vec2 node_size = node_2d->get_size();
            size.x += node_size.x;
            size.y = glm::max(size.y, node_size.y);
        }

        size += padding * 2.0f;
        size.x += item_margin.x * static_cast<float>(child_count - 1);

        Container2D::on_children_changed();
    }

    VContainer2D::VContainer2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Container2D(name, pos, col)
    {
        type = Node2DType::VCONTAINER;
    }

    void VContainer2D::on_children_changed()
    {
        size_t child_count = get_children().size();
        size = glm::vec2(0.0f);

        for (size_t i = 0; i < child_count; ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            node_2d->set_translation(padding + glm::vec2(0.0f, size.y + item_margin.y * static_cast<float>(i)));

            glm::vec2 node_size = node_2d->get_size();
            size.x = glm::max(size.x, node_size.x);
            size.y += node_size.y;
        }

        size += padding * 2.0f;
        size.y += item_margin.y * static_cast<float>(child_count - 1);

        Container2D::on_children_changed();
    }

    /*
    *   Text
    */

    Text2D::Text2D(const std::string& _text, const glm::vec2& pos, float scale, const Color& color)
        : Node2D(_text, pos, {1.0f, 1.0f}) {

        type = Node2DType::TEXT;

        text_entity = new TextEntity(_text);
        text_entity->set_scale(scale);
        text_entity->generate_mesh(color, MATERIAL_2D);
        text_entity->translate(glm::vec3(get_translation(), 0.0f));
    }

    void Text2D::render()
    {
        text_entity->render();

        Node2D::render();
    }

    /*
    *	Button
    */

    Button2D::Button2D(const std::string& sg, bool is_color_button, const Color& col)
        : Button2D(sg, {0.0f, 0.0f}, glm::vec2(BUTTON_SIZE), is_color_button, col) { }

    Button2D::Button2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size, bool is_color_button, const Color& col)
        : Panel2D(sg, pos, size, col), signal(sg), is_color_button(is_color_button) {

        type = Node2DType::BUTTON;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D;
        material.priority = type;

        std::vector<std::string> define_specializations;

        if (!is_color_button) {
            define_specializations.push_back("USES_TEXTURE");
            material.diffuse_texture = RendererStorage::get_texture("data/textures/material_samples.png");
            material.flags |= MATERIAL_DIFFUSE;
        }

        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_button.wgsl", material, define_specializations);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y);

        quad = new MeshInstance3D();
        quad->add_surface(quad_surface);
        quad->set_surface_material_override(quad->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, quad, ui_data, is_color_button ? 2 : 3);

        // Submenu icon..
        {
            /*Button2D* submenu_mark = new Button2D(sg + "@mark", {0.f, 0.f}, {0.f, 0.f}, colors::WHITE);
            submenu_mark->add_surface(RendererStorage::get_surface("quad"));
            
            Material material;
            material.diffuse_texture = RendererStorage::get_texture("data/textures/submenu_mark.png");
            material.flags |= MATERIAL_DIFFUSE | MATERIAL_2D;
            material.color = colors::WHITE;
            material.shader = RendererStorage::get_shader("data/shaders/ui/ui_texture.wgsl", material);
            
            submenu_mark->set_surface_material_override(mark->get_surface(0), material);*/
        }
    }

    void Button2D::render()
    {
        Panel2D::render();

        /*if (mark) mark->render();
        if (label) label->render();*/
    }

    void Button2D::update(float delta_time)
	{
        /*if (mark)
        {
            mark->set_model(get_model());
            mark->translate(glm::vec3(0.7f, 0.7f, -1e-3f));
            mark->scale(glm::vec3(0.35f, 0.35f, 1.0f));
        }*/

		// Check hover (intersects)
        bool hovered = is_hovered();

        /*if (!label)
        {
            float magic = 0.002125f;
            label = new TextWidget(signal, { m_position.x - signal.length() * magic, m_position.y - m_scale.y * 0.75f }, 0.01f, colors::WHITE);
            label->m_priority = 2;
            label->uid = uid;
            label->controller = controller;
        }*/

        // label->set_active(hovered);

        // Used to disable presses and active hovers
        hovered &= (!ui_data.is_button_disabled);

		/*
		*	Create mesh and render button
		*/

        bool is_pressed = hovered && Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
        bool was_pressed = hovered && Input::was_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);

        if (was_pressed)
        {
			Node::emit_signal(signal, (void*)this);

            if (selected)
            {
                if (is_color_button)
                {
                    /*if (current_selected) {
                        current_selected->selected = false;
                    }
                    current_selected = this;*/
                }
            }
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.is_selected = selected ? 1.f : 0.f;
        ui_data.is_color_button = is_color_button ? 1.f : 0.f;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, quad, ui_data);

        // label->update(delta_time);

        Panel2D::update(delta_time);
	}

    /*
    *   Widget Group
    */

    ItemGroup2D::ItemGroup2D(const glm::vec2& pos, const Color& color)
        : HContainer2D("button_group", pos, color) {

        type = Node2DType::GROUP;

        ui_data.num_group_items = 0;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D;
        material.priority = type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_group.wgsl", material);

        quad->set_surface_material_override(quad->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, quad, ui_data, 2);
    }

    float ItemGroup2D::get_number_of_items()
    {
        return ui_data.num_group_items;
    }

    void ItemGroup2D::set_number_of_items(float number)
    {
        ui_data.num_group_items = number;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, quad, ui_data);
    }

    void ItemGroup2D::on_children_changed()
    {
        HContainer2D::on_children_changed();

        set_number_of_items(static_cast<float>(get_children().size()));
    }

    /*
    *   Widget Submenu
    */

    ButtonSubmenu2D::ButtonSubmenu2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size)
        : Button2D(sg, pos, size) {

        box = new ui::HContainer2D("h_container", glm::vec2(0.0f, size.y + GROUP_MARGIN));
        box->set_visibility(false);

        // box->centered = true;

        Node2D::add_child(box);

        Node::bind(sg, [box = box](const std::string& sg, void* data) {

            const bool last_value = box->get_visibility();

            for (auto& w : all_widgets)
            {
                ButtonSubmenu2D* submenu = dynamic_cast<ButtonSubmenu2D*>(w.second);

                if (!submenu) // || b->m_layer < widget->m_layer)
                    continue;

                submenu->box->set_visibility(false);
            }

            box->set_visibility(!last_value);
        });
    }

    void ButtonSubmenu2D::add_child(Node2D* child)
    {
        box->add_child(child);
    }

	/*
	*	Slider
	*/

    Slider2D::Slider2D(const std::string& sg, float value, const glm::vec2& pos, int mode)
        : Panel2D(sg, pos, {0.0f, 0.0f}), signal(sg), current_value(value) {

        this->type = Node2DType::SLIDER;
        this->mode = mode;

        ui_data.num_group_items = mode == SliderMode::HORIZONTAL ? 2.f : 1.f;
        size = glm::vec2(BUTTON_SIZE * ui_data.num_group_items, BUTTON_SIZE);

        Material material;
        material.flags = MATERIAL_2D;
        material.priority = type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_slider.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y);

        quad = new MeshInstance3D();
        quad->add_surface(quad_surface);
        quad->set_surface_material_override(quad->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, quad, ui_data, 2);
    }

    void Slider2D::render()
	{
        Panel2D::render();

        /*if(label) label->render();
        if(text_value) text_value->render();*/
	}

	void Slider2D::update(float delta_time)
	{
        /*if (!label || !text_value) {
            create_helpers();
        }*/

        bool hovered = is_hovered();
        // label->set_active(hovered);

        bool is_pressed = hovered && Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);

		if (is_pressed)
		{
            float range = (mode == HORIZONTAL ? size.x : size.y);
            glm::vec2 local_mouse_pos = Input::get_mouse_position() - get_translation();
            float bounds = range * 0.975f;
            // -scale..scale -> 0..1
            float local_point = (mode == HORIZONTAL ? local_mouse_pos.x : size.y - local_mouse_pos.y);
            // this is at range 0..1
			current_value = glm::clamp(local_point / bounds, 0.f, 1.f);
            // set in range min-max
            current_value = current_value * (max_value - min_value) + min_value;
            if (step == 1.0f) current_value = std::roundf(current_value);
			Node::emit_signal(signal, current_value);
            std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
            // text_value->text_entity->set_text(value_as_string.substr(0, 4));
		}

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.slider_value = current_value;
        ui_data.slider_max = max_value;

        auto webgpu_context = Renderer::instance->get_webgpu_context();

        RendererStorage::update_ui_widget(webgpu_context, quad, ui_data);

        /*label->update(delta_time);
        text_value->update(delta_time);*/

        Panel2D::update(delta_time);
	}

    void Slider2D::set_value(float new_value)
    {
        /*if (!text_value) {
            create_helpers();
        }*/

        current_value = glm::clamp(new_value, min_value, max_value);
        if (step == 1.0f) current_value = std::roundf(current_value);
        std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
        // text_value->text_entity->set_text(value_as_string.substr(0, 4));
    }

    void Slider2D::create_helpers()
    {
        float magic_t = 0.002125f;
        float magic_c = 0.005f;

        /*if (!label)
        {
            label = new TextWidget(signal, { m_position.x - signal.length() * magic_t, m_position.y + m_scale.y * 0.5f }, 0.01f, colors::WHITE);
            label->m_priority = 2;
            label->uid = uid;
            label->controller = controller;
        }

        if (!text_value)
        {
            std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
            text_value = new TextWidget(value_as_string.substr(0, 4), { m_position.x - 4 * magic_t, m_position.y - magic_c }, 0.01f, colors::WHITE);
            text_value->m_priority = 2;
            text_value->uid = uid;
            text_value->controller = controller;
        }*/
    }

    /*
    *	ColorPicker
    */

    ColorPicker2D::ColorPicker2D(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c)
        : Panel2D(sg, p, s, c), signal(sg)
    {
        type = Node2DType::COLOR_PICKER;

        Material material;
        material.flags = MATERIAL_2D;
        material.priority = type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_color_picker.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y);

        quad = new MeshInstance3D();
        quad->add_surface(quad_surface);
        quad->set_surface_material_override(quad->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, quad, ui_data, 2);
    }

    void ColorPicker2D::render()
    {
        Panel2D::render();
    }

    void ColorPicker2D::update(float delta_time)
    {
        bool hovered = is_hovered();

        glm::vec2 local_mouse_pos = Input::get_mouse_position() - get_translation();
        local_mouse_pos /= size;
        local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1
        float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

        // Update hover
        hovered &= (dist < 1.f);

        bool is_pressed = hovered && Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
        bool was_released = hovered && Input::was_mouse_released(GLFW_MOUSE_BUTTON_LEFT);

        if (is_pressed)
        {
            constexpr float pi = glm::pi<float>();
            float r = pi / 2.f;
            local_mouse_pos = glm::mat2x2(cos(r), -sin(r), sin(r), cos(r)) * local_mouse_pos;
            glm::vec2 polar = glm::vec2(atan2(local_mouse_pos.y, local_mouse_pos.x), glm::length(local_mouse_pos));
            float percent = (polar.x + pi) / (2.0f * pi);
            glm::vec3 hsv = glm::vec3(percent, 1.0f, polar.y);

            // Store it without conversion and intensity multiplier
            color = glm::vec4(hsv2rgb(hsv), color.a);
            // Send the signal using the final color
            glm::vec3 new_color = glm::pow(glm::vec3(color) * color.a, glm::vec3(2.2f));
            Node::emit_signal(signal, glm::vec4(new_color, color.a));
        }

        if (was_released)
        {
            Node::emit_signal(signal + "@released", color);
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.picker_color = color;

        auto webgpu_context = Renderer::instance->get_webgpu_context();

        RendererStorage::update_ui_widget(webgpu_context, quad, ui_data);

        Panel2D::update(delta_time);
    }

    /*
    *   Label
    */

    /*LabelWidget::LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s) : UIEntity(p, s), text(p_text) {
        type = eWidgetType::LABEL;
        center_pos = false;
    }*/
}
