#include "ui_controller.h"

#include "framework/utils/utils.h"
#include "framework/input.h"
#include "framework/utils/intersections.h"
#include "framework/nodes/ui.h"
#include "framework/nodes/text.h"
#include "framework/scene/parse_scene.h"

#include "json.hpp"

#include "spdlog/spdlog.h"

namespace ui {

	void Controller::set_workspace(glm::vec2 _workspace_size, uint8_t _select_button, uint8_t _root_pose, uint8_t _hand, uint8_t _select_hand)
	{
		global_scale = 0.001f;

		workspace = {
			.size = _workspace_size * global_scale,
			.select_button = _select_button,
			.root_pose = _root_pose,
			.hand = _hand,
			.select_hand = _select_hand
		};

        root = new Node2D();
        // ((Entity*)root)->set_process_children(true);

        raycast_pointer = parse_mesh("data/meshes/raycast.obj");

        Material pointer_material;
        pointer_material.shader = RendererStorage::get_shader("data/shaders/ui/ui_ray_pointer.wgsl", pointer_material);

        raycast_pointer->set_surface_material_override(raycast_pointer->get_surface(0), pointer_material);
	}

	bool Controller::is_active()
	{
        return workspace.hand == HAND_RIGHT || Input::is_button_pressed(XR_BUTTON_X);
	}

	void Controller::render()
	{
		if (!enabled || !is_active()) return;

        root->render();

        if(workspace.hand == HAND_LEFT)
		    raycast_pointer->render();
	}

	void Controller::update(float delta_time)
	{
		if (!enabled || !is_active()) return;

		uint8_t hand = workspace.hand;
		uint8_t pose = workspace.root_pose;

		// Update raycast helper

        if (hand == HAND_LEFT)
        {
		    glm::mat4x4 raycast_transform = Input::get_controller_pose(workspace.select_hand, pose);
		    raycast_pointer->set_model(raycast_transform);
        }

		// Update workspace

        global_transform = Input::get_controller_pose(hand, pose);
        global_transform = glm::rotate(global_transform, glm::radians(120.f), glm::vec3(1.f, 0.f, 0.f));

        if (pose == POSE_GRIP)
            global_transform = glm::rotate(global_transform, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));

		// Update widgets using this controller (not the root!)

        for (auto child : root->get_children()) {
            child->update(delta_time);
        }
	}

 //   UIEntity* Controller::make_label(const json* j)
 //   {
 //       std::string text = (*j)["name"];
 //       std::string alias = (*j).value("alias", text);

 //       static int num_labels = 0;

 //       // World attributes
 //       float workspace_width = workspace.size.x / global_scale;
 //       float margin = 2.f;

 //       // Text follows after icon (right)
 //       glm::vec2 pos = { LABEL_BUTTON_SIZE + 4.f, num_labels * LABEL_BUTTON_SIZE + (num_labels + 1.f) * margin };
 //       glm::vec2 size = glm::vec2(workspace_width, LABEL_BUTTON_SIZE);

 //       UIEntity* text_widget = make_text(text, "text@" + alias, pos, colors::WHITE, 12.f);
 //       text_widget->m_priority = -1;
 //       // ((Entity*)text_widget)->set_process_children(true);
 //       text_widget->center_pos = false;

 //       // Icon goes to the left of the workspace
 //       pos = { 
 //           -workspace_width * 0.5f + LABEL_BUTTON_SIZE * 0.5f,
 //           num_labels * LABEL_BUTTON_SIZE + (num_labels + 1.f) * margin
 //       };

 //       process_params(pos, size);

 //       // Icon 
 //       LabelWidget* m_icon = new LabelWidget(text, pos, glm::vec2(size.y, size.y));
 //       m_icon->add_surface(RendererStorage::get_surface("quad"));

 //       Material material;

 //       if (j->count("texture") > 0)
 //       {
 //           std::string texture = (*j)["texture"];
 //           material.diffuse_texture = RendererStorage::get_texture(texture);
 //           material.flags |= MATERIAL_DIFFUSE;
 //       }
 //       material.shader = RendererStorage::get_shader("data/shaders/ui/ui_texture.wgsl", material);

 //       m_icon->set_surface_material_override(m_icon->get_surface(0), material);

 //       m_icon->button = j->value("button", -1);
 //       m_icon->subtext = j->value("subtext", "");

 //       if (m_icon->button != -1)
 //       {
 //           bind(m_icon->button, [widget = m_icon, text_widget = text_widget]() {
 //               widget->selected = !widget->selected;
 //               static_cast<ui::TextWidget*>(text_widget)->set_text(widget->selected ? widget->subtext : widget->text);
 //           });
 //       }

 //       append_widget(m_icon, alias, text_widget);

 //       num_labels++;

 //       return m_icon;
 //   }
}
