#pragma once

#include "framework/colors.h"
#include "node_2d.h"
#include "mesh_instance_3d.h"
#include "text.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

	class Controller;

    class Panel2D : public Node2D {
    public:

        Color color;

        MeshInstance3D* quad = nullptr;

        Panel2D(const glm::vec2& p, const glm::vec2& s, const Color& c = colors::WHITE);

        void render() override;
    };

    class Text2D : public Node2D {
    public:

        TextEntity* text_entity = nullptr;

        Text2D(const std::string& _text, const glm::vec2& pos, float scale = 1.f, const Color& color = colors::WHITE);

        void set_text(const std::string& text) { text_entity->set_text(text); };

        void render() override;
    };

	/*class UIEntity : public MeshInstance3D {
	public:

        UIEntity();
        UIEntity(const glm::vec2& p, const glm::vec2& s = {1.f, 1.f});

        static UIEntity* current_selected;

        Controller* controller = nullptr;

        bool is_submenu         = false;
        bool center_pos         = true;

        glm::vec2   m_position = { 0.f, 0.f };
        glm::vec2   m_scale = { 1.f, 1.f };

        uint8_t     m_layer = 0;
		int         m_priority = 0;
        void set_process_children(bool value, bool force = false);
        void set_selected(bool value);
        void set_layer(uint8_t l) { m_layer = l; };
        void set_ui_priority(int p) { m_priority = p; }

        const glm::vec2 position_to_world(const glm::vec2& workspace_size) { return m_position + workspace_size - m_scale;  };
        bool is_hovered(glm::vec3& intersection);

		void update(float delta_time) override;
	};

    class ButtonGroup2D : public Node2D {
    public:

        ButtonGroup2D(const glm::vec2& p, const glm::vec2& s, float number_of_widgets);

        float get_number_of_widgets();
        void set_number_of_widgets(float number);
    };

	

    class LabelWidget : public UIEntity {
    public:

        int button = -1;

        std::string text;
        std::string subtext;

        LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s = {0.f, 0.f});
    };*/

	//class Button2D : public Node2D {
	//public:

 //       // TextWidget* label = nullptr;
 //       Color color;
	//	std::string signal;

 //       // ButtonWidget* mark = nullptr;

 //       bool is_color_button        = false;
 //       bool is_unique_selection    = false;
 //       bool allow_toggle           = false;
 //       bool selected               = false;

 //       Button2D(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c, bool is_color_button = false);

 //       void render() override;
	//    void update(float delta_time) override;
	//};

	/*class SliderWidget : public UIEntity {
	public:

        TextWidget* label = nullptr;
        TextWidget* text_value = nullptr;

        Color color;
        std::string signal;

        enum {
            HORIZONTAL,
            VERTICAL
        };

        int mode = HORIZONTAL;

        float current_value = 0.0f;
        float min_value     = 0.0f;
        float max_value     = 1.0f;
        float step          = 0.0f;

		SliderWidget(const std::string& sg, float v, const glm::vec2& p, const glm::vec2& s, const Color& c, int mode = VERTICAL);

        void render() override;
        void update(float delta_time) override;

        void set_value(float new_value);
        void create_helpers();
	};

    class ColorPickerWidget : public UIEntity {
	public:

		Color current_color;
        std::string signal;

		ColorPickerWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c);

        void set_color(const Color& c);

        void update(float delta_time) override;
	};*/
}
