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

        // Debug
        bool render_background      = false;
        MeshInstance3D* background = nullptr;
        // ...

        MeshInstance3D* raycast_pointer = nullptr;

		Node2D* root = nullptr;

        json* mjson = nullptr;
        std::map<std::string, Node2D*> widgets;
        static std::map<std::string, Node2D*> all_widgets;

		/*
		*	Widget Helpers
		*/

        bool dirty = false;
        bool group_opened = false;

        float g_iterator = 0.f;

        glm::vec2 layout_iterator = { 0.f, 0.f };
        glm::vec2 last_layout_pos;

        std::map<unsigned int, float> layers_width;

		std::vector<Node2D*> parent_queue;

		// void append_widget(Node2D* widget, const std::string& name, Node2D* force_parent = nullptr);
		void process_params(glm::vec2& position, glm::vec2& size, bool skip_to_local = false);
        // glm::vec2 compute_position(float xOffset = 1.f);

	public:

        ~Controller();

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

		/*
		*	Widgets
		*/

       /* Node2D* make_rect(glm::vec2 pos, glm::vec2 size, const Color& color);
		Node2D* make_text(const std::string& text, const std::string& alias, glm::vec2 pos, const Color& color, float scale = 1.f, glm::vec2 size = {1, 1});
        Node2D* make_label(const json* j);
		Node2D* make_button(const json* j);
        Node2D* make_button(const std::string& signal, const std::string& texture, const Color& color = colors::WHITE, bool selected = false, bool unique_selection = true,
                                    bool allow_toggle = false, bool is_color_button = false, bool disabled = false, bool keep_rgb = false);
		Node2D* make_slider(const json* j, const std::string& force_name = "");
		Node2D* make_color_picker(const json* j);
        void make_submenu(Node2D* parent, const std::string& name);
        void close_submenu();

        Node2D* make_group(const json* j);*/
        void close_group();
        void set_next_parent(Node2D* parent);

        const std::map<std::string, Node2D*>& get_widgets() { return widgets; };

        // float get_layer_width(unsigned int uid);

        void load_layout(const std::string& filename);
        void change_list_layout(const std::string& list_name);
	};
}
