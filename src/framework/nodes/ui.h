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

    const float BUTTON_SIZE = 128.f;
    const float GROUP_MARGIN = 16.f;

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

    class Button2D : public Node2D {
    public:

        // TextWidget* label = nullptr;

        MeshInstance3D* quad = nullptr;
        std::string     signal;
        Color           color;

        // ButtonWidget* mark = nullptr;

        bool is_color_button        = false;
        bool is_unique_selection    = false;
        bool allow_toggle           = false;
        bool selected               = false;

        Button2D(const std::string& sg, bool is_color_button = false, const Color& color = colors::WHITE);
        Button2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE), bool is_color_button = false, const Color& color = colors::WHITE);

        void render() override;
        void update(float delta_time) override;
    };

    class ButtonGroup2D : public Node2D {
    public:

        glm::vec2 item_size;

        MeshInstance3D* quad = nullptr;

        ButtonGroup2D(const glm::vec2& pos, const glm::vec2& item_size);

        float get_number_of_items();
        void set_number_of_items(float number);

        void render() override;
        void update(float delta_time) override;
        void add_child(Node2D* child) override;
    };

    enum SliderMode {
        HORIZONTAL,
        VERTICAL
    };

    class Slider2D : public Node2D {
    public:

        /*TextWidget* label = nullptr;
        TextWidget* text_value = nullptr;*/

        MeshInstance3D* quad = nullptr;
        std::string     signal;

        int mode = SliderMode::HORIZONTAL;

        float current_value = 0.0f;
        float min_value = 0.0f;
        float max_value = 1.0f;
        float step = 0.0f;

        Slider2D(const std::string& sg, float v, const glm::vec2& pos, int mode = SliderMode::VERTICAL);

        void render() override;
        void update(float delta_time) override;

        void set_value(float new_value);
        void create_helpers();
    };

    class ColorPicker2D : public Node2D {
    public:

        MeshInstance3D* quad = nullptr;
        std::string     signal;
        Color           current_color;

        ColorPicker2D(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c);

        void set_color(const Color& c);

        void render() override;
        void update(float delta_time) override;
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

		void update(float delta_time) override;
	};
	

    class LabelWidget : public UIEntity {
    public:

        int button = -1;

        std::string text;
        std::string subtext;

        LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s = {0.f, 0.f});
    };*/
}
