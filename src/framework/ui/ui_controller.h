#pragma once

#include "framework/colors.h"
#include "framework/input_xr.h"
#include "framework/utils/json_utils.h"

#include <variant>
#include <functional>
#include <map>
#include <string>
#include <vector>

class EntityMesh;

namespace ui {
    class UIEntity;
}

using FuncEmpty = std::function<void()>;

using FuncVoid = std::function<void(const std::string&, void*)>;
using FuncFloat = std::function<void(const std::string&, float)>;
using FuncString = std::function<void(const std::string&, std::string)>;
using FuncVec2 = std::function<void(const std::string&, glm::vec2)>;
using FuncVec3 = std::function<void(const std::string&, glm::vec3)>;
using FuncVec4 = std::function<void(const std::string&, glm::vec4)>;

using SignalType = std::variant <FuncFloat, FuncString, FuncVec2, FuncVec3, FuncVec4, FuncVoid>;

namespace ui {

    const float BUTTON_SIZE         = 32.f;
    const float LABEL_BUTTON_SIZE   = BUTTON_SIZE * 0.25f;
    const float X_MARGIN            = 4.f;
    const float X_GROUP_MARGIN      = X_MARGIN * 0.5f;
    const float Y_MARGIN            = 10.f;

    const int   MAX_LAYERS          = 255;

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
        EntityMesh* background      = nullptr;
        // ...

		EntityMesh* raycast_pointer = nullptr;

		ui::UIEntity* root = nullptr;
		std::map<std::string, std::vector<SignalType>> mapping_signals;
        std::map<uint8_t, std::vector<FuncEmpty>> controller_signals;

        json* mjson = nullptr;
        std::map<std::string, UIEntity*> widgets;
        static std::map<std::string, UIEntity*> all_widgets;

		/*
		*	Widget Helpers
		*/

        bool dirty = false;
        bool group_opened = false;

        float g_iterator = 0.f;

        glm::vec2 layout_iterator = { 0.f, 0.f };
        glm::vec2 last_layout_pos;

        std::map<unsigned int, float> layers_width;

		std::vector<UIEntity*> parent_queue;

		void append_widget(UIEntity* widget, const std::string& name, UIEntity* force_parent = nullptr);
		void process_params(glm::vec2& position, glm::vec2& size, bool skip_to_local = false);
        glm::vec2 compute_position(float xOffset = 1.f);

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

        UIEntity* make_rect(glm::vec2 pos, glm::vec2 size, const Color& color);
		UIEntity* make_text(const std::string& text, const std::string& alias, glm::vec2 pos, const Color& color, float scale = 1.f, glm::vec2 size = {1, 1});
        UIEntity* make_label(const json* j);
		UIEntity* make_button(const json* j);
        UIEntity* make_button(const std::string& signal, const std::string& texture, const Color& color = colors::WHITE, bool selected = false, bool unique_selection = true,
                                    bool allow_toggle = false, bool is_color_button = false, bool disabled = false, bool keep_rgb = false);
		UIEntity* make_slider(const json* j, const std::string& force_name = "");
		UIEntity* make_color_picker(const json* j);
        void make_submenu(UIEntity* parent, const std::string& name);
        void close_submenu();

        UIEntity* make_group(const json* j);
        void close_group();
        void set_next_parent(UIEntity* parent);

        const std::map<std::string, UIEntity*>& get_widgets() { return widgets; };
        static UIEntity* get(const std::string& alias);
        UIEntity* get_widget_from_name(const std::string& alias);
        float get_layer_width(unsigned int uid);

        void load_layout(const std::string& filename);
        void change_list_layout(const std::string& list_name);

		/*
		*	Callbacks
		*/

		void bind(const std::string& name, SignalType callback);
        void bind(uint8_t button, FuncEmpty callback);

		template<typename T>
		bool emit_signal(const std::string& name, T value) {

			auto it = mapping_signals.find(name);
			if (it == mapping_signals.end())
				return false;

			using FuncT = std::function<void(const std::string&, T)>;

			for (auto& f : mapping_signals[name])
			{
				if (std::holds_alternative<FuncT>(f))
					std::get<FuncT>(f)(name, value);
			}

			return true;
		}
	};
}
