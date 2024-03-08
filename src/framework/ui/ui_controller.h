#pragma once

#include "framework/colors.h"
#include "framework/input_xr.h"
#include "framework/nodes/node_2d.h"
#include "framework/utils/json_utils.h"
#include "framework/math.h"

#include <variant>
#include <functional>
#include <map>
#include <string>
#include <vector>

class MeshInstance3D;

namespace ui {

	struct WorkSpaceData {
		glm::vec2 size;
		uint8_t select_button;
		uint8_t root_pose;
		uint8_t hand;
		uint8_t select_hand;
	};

	class Controller {

		WorkSpaceData workspace;
		glm::mat4x4 global_transform;

        MeshInstance3D* raycast_pointer = nullptr;

		Node2D* root = nullptr;

	public:

		float global_scale = 1.f;
		bool enabled = true;

		/*
		*	Select button: XR Buttons
		*	Root pose: AIM, GRIP
		*	Hand: To set UI panel
		*	Select hand: Raycast hand
		*/

		WorkSpaceData& get_workspace() { return workspace; };
		void set_workspace(glm::vec2 _workspace_size, uint8_t _select_button = 0, uint8_t _root_pose = POSE_AIM, uint8_t _hand = 0, uint8_t _select_hand = 1);
		const glm::mat4x4& get_matrix() { return global_transform; };
		bool is_active();

		void render();
		void update(float delta_time);
	};
}
