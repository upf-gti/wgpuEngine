#include "entity_ui.h"
#include "utils.h"
#include "framework/ui/ui_controller.h"
#include "framework/intersections.h"
#include "framework/input.h"
#include "framework/entities/entity_text.h"
#include "graphics/renderer.h"
#include <iostream>

namespace ui {

    UIEntity* UIEntity::current_selected = nullptr;
    unsigned int UIEntity::last_uid = 0;

    UIEntity::UIEntity()
    {
        uid = last_uid++;
    }

    UIEntity::UIEntity(const glm::vec2& p, const glm::vec2& s) : m_position(p), m_scale(s)
    {
        uid = last_uid++;
    }

    void UIEntity::set_process_children(bool value, bool force)
    {
        ButtonWidget* bw = dynamic_cast<ButtonWidget*>(this);
        if (bw && bw->is_submenu) {
            process_children = value;
            selected = value;
        }

        // Only hiding is recursive...
        if (!value || force)
        {
            for (auto c : children)
                static_cast<UIEntity*>(c)->set_process_children(value, force);
        }
    }

    void UIEntity::set_selected(bool value)
    {
        selected = value;

        // Only unselecting is recursive...
        if (!value)
        {
            for (auto c : children)
                static_cast<UIEntity*>(c)->set_selected(value);
        }
    }

    bool UIEntity::is_hovered(Controller* controller, glm::vec3& intersection)
    {

        const WorkSpaceData& workspace = controller->get_workspace();

        /*
        *	Manage intersection
        */

        uint8_t hand = workspace.hand;
        uint8_t select_hand = workspace.select_hand;
        uint8_t pose = workspace.root_pose;

        // Ray
        glm::vec3 ray_origin = Input::get_controller_position(select_hand, pose);
        glm::mat4x4 select_hand_pose = Input::get_controller_pose(select_hand, pose);
        glm::vec3 ray_direction = get_front(select_hand_pose);

        // Quad
        glm::vec3 slider_quad_position = get_local_translation();
        glm::quat quad_rotation = glm::quat_cast(controller->get_matrix());

        float collision_dist;

        // Check hover with slider background to move thumb
        return intersection::ray_quad(
            ray_origin,
            ray_direction,
            slider_quad_position,
            m_scale,
            quad_rotation,
            intersection,
            collision_dist
        );
    }

	void UIEntity::render_ui()
	{
        if (!active) return;

		EntityMesh::render();

		if (!process_children)
			return;

		for (auto c : children)
            static_cast<UIEntity*>(c)->render_ui();
    }

	void UIEntity::update_ui(Controller* controller)
	{
        if (!active) return;

        float pos_x = center_pos ?
            m_position.x + controller->get_workspace().size.x - controller->get_layer_width( this->uid )
            : m_position.x;

		set_model(controller->get_matrix());
		translate(glm::vec3(pos_x, m_position.y, -1e-3f - m_priority * 1e-3f));
        scale(glm::vec3(m_scale.x, m_scale.y, 1.f));

		if (!process_children)
			return;

        for (auto c : children)
			static_cast<UIEntity*>(c)->update_ui(controller);
	}

    /*
    *   Widget Group
    */

    WidgetGroup::WidgetGroup(const glm::vec2& p, const glm::vec2& s, float n) : UIEntity(p, s) {

        process_children = true;
        type = eWidgetType::GROUP;
        set_material_flag(MATERIAL_UI);

        ui_data.num_group_items = n;
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, RendererStorage::get_shader("data/shaders/ui/ui_group.wgsl"), this, ui_data, 2);
    }

    /*
    *   Text
    */

    TextWidget::TextWidget(const std::string& _text, const glm::vec2& pos, float scale, const Color& color) : UIEntity(pos) {

        type = eWidgetType::TEXT;
        set_material_flag(MATERIAL_UI);

        text_entity = new TextEntity(_text);
        text_entity->set_material_color(color);
        text_entity->set_scale(scale);
        text_entity->generate_mesh();
    }

    void TextWidget::render_ui()
    {
        if (!active) return;

        UIEntity::render_ui();
        text_entity->render();
    }

    void TextWidget::update_ui(Controller* controller)
    {
        if (!active) return;

        UIEntity::update_ui(controller);

        float pos_x = center_pos ?
            m_position.x + controller->get_workspace().size.x - controller->get_layer_width(this->uid)
            : m_position.x;

        text_entity->set_model(controller->get_matrix());
        text_entity->translate(glm::vec3(pos_x, m_position.y, -1e-3f - m_priority * 1e-3f));
        text_entity->scale(glm::vec3(m_scale.x, m_scale.y, 1.f));
    }

    /*
    *   Label
    */

    LabelWidget::LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s) : UIEntity(p, s), text(p_text) {
        type = eWidgetType::LABEL;
        center_pos = false;
    }

	/*
	*	Button
	*/

    ButtonWidget::ButtonWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c) : UIEntity(p, s), signal(sg), color(c) {

        type = eWidgetType::BUTTON;
        set_material_flag(MATERIAL_UI);
        set_material_color(color);

        float magic = 0.002125f;
        label = new TextWidget(sg, { p.x - sg.length() * magic, p.y + s.y * 0.5f }, 0.01f, colors::GRAY);
        label->m_priority = 2;
        label->uid = uid;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, RendererStorage::get_shader("data/shaders/ui/ui_button.wgsl"), this, ui_data, 3);
    }

    void ButtonWidget::render_ui()
    {
        UIEntity::render_ui();
        static_cast<UIEntity*>(label)->render_ui();
    }

	void ButtonWidget::update_ui(Controller* controller)
	{
        UIEntity::update_ui(controller);

		const WorkSpaceData& workspace = controller->get_workspace();

		// Check hover (intersects)
        glm::vec3 intersection;
        bool hovered = is_hovered(controller, intersection);

        // Used to disable presses and hovers
        hovered &= allow_events;

        label->active = hovered;

		/*
		*	Create mesh and render button
		*/

		bool is_pressed = hovered && Input::is_button_pressed(workspace.select_button);
		bool was_pressed = hovered && Input::was_button_pressed(workspace.select_button);

        if (was_pressed)
        {
			controller->emit_signal(signal, (void*)this);

            if (selected)
            {
                if (is_color_button)
                {
                    if(current_selected) current_selected->selected = false;
                    current_selected = this;
                }
            }
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.is_selected = selected ? 1.f : 0.f;
        ui_data.is_color_button = is_color_button ? 1.f : 0.f;

        auto webgpu_context = Renderer::instance->get_webgpu_context();

        RendererStorage::update_ui_widget(webgpu_context, this, ui_data);

        label->update_ui(controller);
	}

	/*
	*	Slider
	*/

    SliderWidget::SliderWidget(const std::string& sg, float v, const glm::vec2& p, const glm::vec2& s, const Color& c)
        : UIEntity(p, s), signal(sg), current_value(v), color(c) {

        type = eWidgetType::SLIDER;
        set_material_flag(MATERIAL_UI);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, RendererStorage::get_shader("data/shaders/ui/ui_slider.wgsl"), this, ui_data, 3);
    }

    void SliderWidget::render_ui()
	{
        UIEntity::render_ui();
        if(label) static_cast<UIEntity*>(label)->render_ui();
        if(text_value) static_cast<UIEntity*>(text_value)->render_ui();
	}

	void SliderWidget::update_ui(Controller* controller)
	{
		const WorkSpaceData& workspace = controller->get_workspace();

		/*
		*	Update elements
		*/

        UIEntity::update_ui(controller);

        float magic_t = 0.002125f;
        float magic_c = 0.005f;

        if (!label)
        {
            label = new TextWidget(signal, { m_position.x - signal.length() * magic_t, m_position.y + m_scale.y * 0.5f }, 0.01f, colors::WHITE);
            label->m_priority = 2;
            label->uid = uid;
        }

        if (!text_value)
        {
            std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
            text_value = new TextWidget(value_as_string.substr(0, 4), {m_position.x - 4 * magic_t, m_position.y - magic_c }, 0.01f, colors::WHITE);
            text_value->m_priority = 2;
            text_value->uid = uid;
        }

        glm::vec3 intersection;
        bool hovered = is_hovered(controller, intersection);
        label->active = hovered;

		bool is_pressed = hovered && Input::is_button_pressed(workspace.select_button);

		if (is_pressed)
		{
            float range = (mode == HORIZONTAL ? m_scale.x : m_scale.y);
            float bounds = range * 0.975f;
            // -scale..scale -> 0..1
            float local_point = (mode == HORIZONTAL ? intersection.x : -intersection.y);
            local_point = glm::max(glm::min(local_point, bounds), -bounds);
			current_value = glm::clamp((local_point / bounds) * 0.5f + 0.5f, 0.f, 1.f);
			controller->emit_signal(signal, current_value);
            std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
            text_value->text_entity->set_text(value_as_string.substr(0, 4));
		}

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.slider_value = current_value;

        auto webgpu_context = Renderer::instance->get_webgpu_context();

        RendererStorage::update_ui_widget(webgpu_context, this, ui_data);

        label->update_ui(controller);
        text_value->update_ui(controller);
	}

    /*
    *	ColorPicker
    */

    ColorPickerWidget::ColorPickerWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c)
        : UIEntity(p, s), signal(sg), current_color(c)
    {
        type = eWidgetType::COLOR_PICKER;
        set_material_flag(MATERIAL_UI);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, RendererStorage::get_shader("data/shaders/ui/ui_color_picker.wgsl"), this, ui_data, 3);
    }

    void ColorPickerWidget::update_ui(Controller* controller)
    {
        const WorkSpaceData& workspace = controller->get_workspace();

        /*
        *	Update elements
        */

        UIEntity::update_ui(controller);

        glm::vec3 intersection;
        bool hovered = is_hovered(controller, intersection);
        bool is_pressed = hovered && Input::is_button_pressed(workspace.select_button);
        bool was_released = hovered && Input::was_button_released(workspace.select_button);

        glm::vec2 local_point = glm::vec2(intersection);

        if (is_pressed)
        {
            glm::vec2 bounds = m_scale * 0.975f;
            local_point = glm::max(glm::min(local_point, bounds), -bounds) / bounds; // -1..1

            constexpr float pi = glm::pi<float>();
            float r = pi / 2.f;
            local_point = glm::mat2x2(cos(r), -sin(r), sin(r), cos(r)) * local_point;
            glm::vec2 polar = glm::vec2(atan2(local_point.y, local_point.x), glm::length(local_point));
            float percent = (polar.x + pi) / (2.0 * pi);
            glm::vec3 hsv = glm::vec3(percent, 1., polar.y);

            current_color = glm::vec4(hsv2rgb(hsv), current_color.a);
            controller->emit_signal(signal, current_color);
        }

        if (was_released)
        {
            controller->emit_signal(signal + "@released", current_color);
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.picker_color = current_color;

        auto webgpu_context = Renderer::instance->get_webgpu_context();

        RendererStorage::update_ui_widget(webgpu_context, this, ui_data);
    }
}
