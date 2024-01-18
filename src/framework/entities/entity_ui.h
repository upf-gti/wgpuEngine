#pragma once

#include "framework/colors.h"
#include "graphics/webgpu_context.h"
#include "graphics/renderer_storage.h"
#include "entity_mesh.h"
#include "entity_text.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

	class Controller;

	enum eWidgetType {
		NONE,
		TEXT,
		BUTTON,
		SLIDER,
        LABEL,
        COLOR_PICKER,
        GROUP
	};

	class UIEntity : public EntityMesh {
	public:

        UIEntity();
        UIEntity(const glm::vec2& p, const glm::vec2& s = {1.f, 1.f});

        static unsigned int last_uid;

        unsigned int uid;
        uint8_t type = eWidgetType::NONE;

        static UIEntity* current_selected;

        Controller* controller = nullptr;

        bool is_submenu         = false;
        bool selected           = false;
        bool center_pos         = true;

        glm::vec2   m_position = { 0.f, 0.f };
        glm::vec2   m_scale = { 1.f, 1.f };

        uint8_t     m_layer = 0;
		int         m_priority = 0;

        RendererStorage::sUIData ui_data;

        void set_process_children(bool value, bool force = false);
        void set_selected(bool value);
        void set_layer(uint8_t l) { m_layer = l; };

        const glm::vec2 position_to_world(const glm::vec2& workspace_size) { return m_position + workspace_size - m_scale;  };
        bool is_hovered(glm::vec3& intersection);

        glm::mat4x4 get_global_model() const override;

		void update(float delta_time) override;
	};

    class WidgetGroup : public UIEntity {
    public:

        WidgetGroup(const glm::vec2& p, const glm::vec2& s, float number_of_widgets);
    };

	class TextWidget : public UIEntity {
	public:

        TextEntity* text_entity = nullptr;

		TextWidget(const std::string& _text, const glm::vec2& pos, float scale = 1.f, const Color& color = colors::WHITE);

        void set_text(const std::string& text) { text_entity->set_text(text); };

        void render() override;
        void update(float delta_time) override;
	};

    class LabelWidget : public UIEntity {
    public:

        int button = -1;

        std::string text;
        std::string subtext;

        LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s = {0.f, 0.f});
    };

	class ButtonWidget : public UIEntity {
	public:

        TextWidget* label = nullptr;
        Color color;
		std::string signal;

        ButtonWidget* mark = nullptr;

        bool is_color_button        = false;
        bool is_unique_selection    = false;
        bool allow_toggle           = false;
        bool allow_events           = true;

        ButtonWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c);

        void render() override;
	    void update(float delta_time) override;
	};

	class SliderWidget : public UIEntity {
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

		float current_value = 0.f;
        float min_value = 0.f;
        float max_value = 1.f;

		SliderWidget(const std::string& sg, float v, const glm::vec2& p, const glm::vec2& s, const Color& c, int mode = VERTICAL);

        void render() override;
        void update(float delta_time) override;
	};

    class ColorPickerWidget : public UIEntity {
	public:

		Color current_color;
        std::string signal;

		ColorPickerWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c);

        void update(float delta_time) override;
	};
}
