#include "entity_ui.h"
#include "utils.h"
#include "framework/ui/ui_controller.h"
#include "framework/intersections.h"
#include "framework/input.h"
#include "framework/entities/entity_text.h"
#include "graphics/renderer.h"

namespace ui {

    UIEntity* UIEntity::current_selected = nullptr;

    UIEntity::UIEntity(const glm::vec2& p, const glm::vec2& s) : m_position(p), m_scale(s)
    {
        
    }

    void UIEntity::set_show_children(bool value)
    {
        ButtonWidget* bw = dynamic_cast<ButtonWidget*>(this);
        if (bw && bw->is_submenu) {
            show_children = value;
            selected = value;
        }

        // Only hiding is recursive...
        if (!value)
        {
            for (auto c : children)
                static_cast<UIEntity*>(c)->set_show_children(value);
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

	void UIEntity::render_ui()
	{
        if (!active) return;

		EntityMesh::render();

		if (!show_children)
			return;

		for (auto c : children)
            static_cast<UIEntity*>(c)->render_ui();
    }

	void UIEntity::update_ui(Controller* controller)
	{
        if (!active) return;

		set_model(controller->get_matrix());
		translate(glm::vec3(m_position.x, m_position.y, -1e-3f - m_priority * 1e-3f));
        scale(glm::vec3(m_scale.x, m_scale.y, 1.f));

		if (!show_children)
			return;

        for (auto c : children)
			static_cast<UIEntity*>(c)->update_ui(controller);
	}

    /*
    *   Widget Group
    */

    WidgetGroup::WidgetGroup(const glm::vec2& p, const glm::vec2& s, float n) : UIEntity(p, s) {

        show_children = true;
        type = eWidgetType::GROUP;
        set_material_flag(MATERIAL_UI);

        ui_data.num_group_items = n;
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, RendererStorage::get_shader("data/shaders/mesh_ui.wgsl"), this, ui_data, 2);
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

        text_entity->set_model(controller->get_matrix());
        text_entity->translate(glm::vec3(m_position.x, m_position.y, -1e-3f - m_priority * 1e-3f));
        text_entity->scale(glm::vec3(m_scale.x, m_scale.y, 1.f));
    }

    /*
    *   Label
    */

    LabelWidget::LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s) : UIEntity(p, s), text(p_text) {
        type = eWidgetType::LABEL;
    }

	/*
	*	Button
	*/

    ButtonWidget::ButtonWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c) : UIEntity(p, s), signal(sg), color(c) {

        type = eWidgetType::BUTTON;
        set_material_flag(MATERIAL_UI);

        float magic = 0.002125f;
        label = new TextWidget(sg, { p.x - sg.length() * magic, p.y + s.y * 0.5f }, 0.01f, colors::GRAY);
        label->m_priority = 2;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, RendererStorage::get_shader("data/shaders/mesh_texture_ui.wgsl"), this, ui_data, 3);
    }

    void ButtonWidget::render_ui()
    {
        UIEntity::render_ui();
        static_cast<UIEntity*>(label)->render_ui();
    }

	void ButtonWidget::update_ui(Controller* controller)
	{
        UIEntity::update_ui(controller);

		/*
		*	Manage intersection
		*/

		const WorkSpaceData& workspace = controller->get_workspace();

		uint8_t hand = workspace.hand;
		uint8_t select_hand = workspace.select_hand;
		uint8_t pose = workspace.root_pose;

		// Ray
		glm::vec3 ray_origin = Input::get_controller_position(select_hand, pose);
		glm::mat4x4 select_hand_pose = Input::get_controller_pose(select_hand, pose);
		glm::vec3 ray_direction = get_front(select_hand_pose);

		// Quad
		glm::vec3 quad_position = get_local_translation();
		glm::quat quad_rotation = glm::quat_cast(controller->get_matrix());

		// Check hover (intersects)
		glm::vec3 intersection;
		float collision_dist;
		bool hovered = intersection::ray_quad(
			ray_origin,
			ray_direction,
			quad_position,
			m_scale,
			quad_rotation,
			intersection,
			collision_dist
		);

        label->active = hovered;

		/*
		*	Create mesh and render button
		*/

		bool is_pressed = hovered && Input::is_button_pressed(workspace.select_button);
		bool was_pressed = hovered && Input::was_button_pressed(workspace.select_button);

        set_material_color(/*is_pressed ? colors::GRAY : */color);
		
        if (was_pressed)
        {
			controller->emit_signal(signal, (void*)this);

            if (selected && is_color_button)
            {
                if(current_selected) current_selected->selected = false;
                current_selected = this;
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

	//void SliderWidget::render()
	//{
	//	Widget::render();
	//	thumb_entity->render();
	//}

	//void SliderWidget::update(Controller* controller)
	//{
	//	const WorkSpaceData& workspace = controller->get_workspace();

	//	// We need to store this position before converting to local size
	//	glm::vec2 pos = position;
	//	glm::vec2 thumb_size = { scale.y, scale.y };

	//	// No value assigned
	//	if (current_slider_pos == -1)
	//	{
	//		max_slider_pos = (scale.x * 2.f - thumb_size.x * 2.f) / controller->global_scale;
	//		current_slider_pos = current_value * max_slider_pos;
	//	}

	//	float thumb_pos = current_slider_pos * controller->global_scale - workspace.size.x + thumb_size.x + pos.x * 2.f;

	//	// To workspace local size
	//	pos -= (workspace.size - scale - pos);

	//	/*
	//	*	Update elements
	//	*/

	//	entity->set_model(controller->get_matrix());
	//	entity->translate(glm::vec3(pos.x, pos.y, -1e-3f));
 //       entity->translate(glm::vec3(scale.x, scale.y, 0.f));

	//	thumb_entity->set_model(controller->get_matrix());
	//	thumb_entity->translate(glm::vec3(thumb_pos, pos.y, -2e-3f));
 //       thumb_entity->translate(glm::vec3(scale.y, scale.y, 0.f));

	//	/*
	//	*	Manage intersection
	//	*/

	//	uint8_t hand = workspace.hand;
	//	uint8_t select_hand = workspace.select_hand;
	//	uint8_t pose = workspace.root_pose;

	//	// Ray
	//	glm::vec3 ray_origin = Input::get_controller_position(select_hand, pose);
	//	glm::mat4x4 select_hand_pose = Input::get_controller_pose(select_hand, pose);
	//	glm::vec3 ray_direction = get_front(select_hand_pose);

	//	// Quad
	//	glm::vec3 quad_position = thumb_entity->get_translation();
	//	glm::vec3 slider_quad_position = entity->get_translation();
	//	glm::quat quad_rotation = glm::quat_cast(controller->get_matrix());

	//	// Check hover with thumb
	//	glm::vec3 intersection;
	//	float collision_dist;
	//	bool thumb_hovered = intersection::ray_quad(
	//		ray_origin,
	//		ray_direction,
	//		quad_position,
	//		thumb_size,
	//		quad_rotation,
	//		intersection,
	//		collision_dist
	//	);

	//	// Check hover with slider background to move thumb
	//	bool slider_hovered = intersection::ray_quad(
	//		ray_origin,
	//		ray_direction,
	//		slider_quad_position,
 //           scale,
	//		quad_rotation,
	//		intersection,
	//		collision_dist
	//	);

	//	/*
	//	*	Create mesh and render thumb
	//	*/

	//	bool is_pressed = thumb_hovered && Input::is_button_pressed(workspace.select_button);
	//	bool was_pressed = thumb_hovered && Input::was_button_pressed(workspace.select_button);

	//	if (is_pressed)
	//	{
	//		current_slider_pos = glm::clamp((intersection.x + scale.x - thumb_size.x) / controller->global_scale, 0.f, max_slider_pos);
	//		current_value = glm::clamp(current_slider_pos / max_slider_pos, 0.f, 1.f);
	//		controller->emit_signal(signal, current_value);
	//	}
	//}
}
