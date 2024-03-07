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

 //   UIEntity* UIEntity::current_selected = nullptr;

 //   UIEntity::UIEntity()
 //   {
 //       uid = last_uid++;
 //       //process_children = false;
 //   }

 //   UIEntity::UIEntity(const glm::vec2& p, const glm::vec2& s) : m_position(p), m_scale(s)
 //   {
 //       uid = last_uid++;
 //       //process_children = false;
 //   }

 //   void UIEntity::set_process_children(bool value, bool force)
 //   {
 //       Button2D* bw = dynamic_cast<Button2D*>(this);
 //       if (bw && bw->is_submenu) {
 //           //process_children = value;
 //           selected = value;
 //       }

 //       // Only hiding is recursive...
 //       if (!value || force)
 //       {
 //           for (auto c : children)
 //               static_cast<UIEntity*>(c)->set_process_children(value, force);
 //       }
 //   }

 //   void UIEntity::set_selected(bool value)
 //   {
 //       selected = value;

 //       // Only unselecting is recursive...
 //       if (!value)
 //       {
 //           for (auto c : children)
 //               static_cast<UIEntity*>(c)->set_selected(value);
 //       }
 //   }


	//void UIEntity::update(float delta_time)
	//{
 //       //if (!active) return;

 //       float row_width = center_pos ? controller->get_layer_width(this->uid) : 0.f;
 //       float pos_x = center_pos ?
 //           position_to_world(controller->get_workspace().size).x - row_width + m_scale.x
 //           : m_position.x;

 //       set_model(controller->get_matrix());
	//	translate(glm::vec3(pos_x, m_position.y, -1e-3f - m_priority * 1e-3f));
 //       scale(glm::vec3(m_scale.x, m_scale.y, 1.f));

	//	/*if (!process_children)
	//		return;*/

 //       MeshInstance3D::update(delta_time);
	//}

    /*
    *   Label
    */

    /*LabelWidget::LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s) : UIEntity(p, s), text(p_text) {
        type = eWidgetType::LABEL;
        center_pos = false;
    }*/

    /*
    *	Panel
    */

    Panel2D::Panel2D(const glm::vec2& pos, const glm::vec2& size, const Color& col)
        : Node2D(pos, size), color(col)
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

        quad->translate(glm::vec3(get_translation() + size * 0.50f, 0.0f));
    }

    void Panel2D::render()
    {
        quad->render();

        Node2D::render();
    }

    /*
    *   Text
    */

    Text2D::Text2D(const std::string& _text, const glm::vec2& pos, float scale, const Color& color)
        : Node2D(pos, {1.0f, 1.0f}) {

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
        : Node2D(pos, size), signal(sg), color(col), is_color_button(is_color_button) {

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
        quad->render();

        Node2D::render();

        /*if (mark) mark->render();
        if (label) label->render();*/
    }

    void Button2D::update(float delta_time)
	{
        quad->set_translation(glm::vec3(get_translation() + size * 0.50f, 0.0f));

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
			// controller->emit_signal(signal, (void*)this);

            spdlog::info("BUTTON PRESSED");

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

        Node2D::update(delta_time);
	}

    /*
    *   Widget Group
    */

    ButtonGroup2D::ButtonGroup2D(const glm::vec2& pos, const glm::vec2& item_size) : Node2D(pos, item_size) {

        type = Node2DType::GROUP;

        this->item_size = item_size;

        ui_data.num_group_items = 0;

        Material material;
        material.color = colors::GREEN;
        material.flags = MATERIAL_2D;
        material.priority = type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_group.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(0.0f, 0.0f);

        quad = new MeshInstance3D();
        quad->add_surface(quad_surface);
        quad->set_surface_material_override(quad->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, quad, ui_data, 2);
    }

    float ButtonGroup2D::get_number_of_items()
    {
        return ui_data.num_group_items;
    }

    void ButtonGroup2D::set_number_of_items(float number)
    {
        ui_data.num_group_items = number;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, quad, ui_data);
    }

    void ButtonGroup2D::update(float delta_time)
    {
        Node2D::update(delta_time);
    }

    void ButtonGroup2D::render()
    {
        if (get_number_of_items() == 0.0f)
            return;

        quad->render();

        Node2D::render();
    }

    void ButtonGroup2D::add_child(Node2D* child)
    {
        Node2D::add_child(child);

        float num_items = get_number_of_items();
        child->set_translation({ GROUP_MARGIN + num_items * item_size.x + GROUP_MARGIN * num_items, GROUP_MARGIN });

        num_items++;
        set_number_of_items(num_items);

        // Recreate quad using new size and reposition accordingly

        glm::vec2 group_size = { num_items * item_size.x + GROUP_MARGIN * (num_items - 1), item_size.y };
        group_size += GROUP_MARGIN * 2.0f;

        Surface* quad_surface = quad->get_surface(0);
        quad_surface->create_quad(group_size.x, group_size.y);

        glm::vec2 new_pos = { get_translation() + group_size * 0.50f};

        quad->set_translation(glm::vec3(new_pos, 0.0f));
    }

	/*
	*	Slider
	*/

    Slider2D::Slider2D(const std::string& sg, float value, const glm::vec2& pos, int mode)
        : Node2D(pos, {0.0f, 0.0f}), signal(sg), current_value(value) {

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
        quad->render();

        Node2D::render();

        /*if(label) label->render();
        if(text_value) text_value->render();*/
	}

	void Slider2D::update(float delta_time)
	{
        quad->set_translation(glm::vec3(get_translation() + size * 0.50f, 0.0f));

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
			// controller->emit_signal(signal, current_value);
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

        Node2D::update(delta_time);
	}

    void Slider2D::set_value(float new_value)
    {
        /*if (!text_value) {
            create_helpers();
        }

        current_value = glm::clamp(new_value, min_value, max_value);
        if (step == 1.0f) current_value = std::roundf(current_value);
        std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
        text_value->text_entity->set_text(value_as_string.substr(0, 4));*/
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
        : Node2D(p, s), signal(sg), current_color(c)
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

    void ColorPicker2D::set_color(const Color& c)
    {
        current_color = c;

        quad->set_surface_material_override_color(0, c);
    }

    void ColorPicker2D::render()
    {
        quad->render();

        Node2D::render();
    }

    void ColorPicker2D::update(float delta_time)
    {
        quad->set_translation(glm::vec3(get_translation() + size * 0.50f, 0.0f));

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
            current_color = glm::vec4(hsv2rgb(hsv), current_color.a);
            // Send the signal using the final color
            glm::vec3 new_color = glm::pow(glm::vec3(current_color) * current_color.a, glm::vec3(2.2f));
            // controller->emit_signal(signal, glm::vec4(new_color, current_color.a));
        }

        if (was_released)
        {
            // controller->emit_signal(signal + "@released", current_color);
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.picker_color = current_color;

        auto webgpu_context = Renderer::instance->get_webgpu_context();

        RendererStorage::update_ui_widget(webgpu_context, quad, ui_data);

        Node2D::update(delta_time);
    }
}
