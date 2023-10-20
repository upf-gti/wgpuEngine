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

        UIEntity() {}
        UIEntity(const glm::vec2& p, const glm::vec2& s = {1.f, 1.f});

        uint8_t type = eWidgetType::NONE;

        static UIEntity* current_selected;

		bool render_children  = false;
        bool update_children = false;
        bool active         = true;
        bool selected       = false;

        glm::vec2   m_position = { 0.f, 0.f };
        glm::vec2   m_scale = { 1.f, 1.f };

        uint8_t     m_layer = 0;
		int         m_priority = 0;

        RendererStorage::sUIData ui_data;

        void set_render_children(bool value, bool force = false);
        void set_selected(bool value);

        bool is_hovered(Controller* controller, glm::vec3& intersection);

		virtual void render_ui();
		virtual void update_ui(Controller* controller);
	};

    class WidgetGroup : public UIEntity {
    public:

        WidgetGroup(const glm::vec2& p, const glm::vec2& s, float number_of_widgets);

        virtual void render_ui() override;
    };

	class TextWidget : public UIEntity {
	public:

        TextEntity* text_entity = nullptr;

		TextWidget(const std::string& _text, const glm::vec2& pos, float scale = 1.f, const Color& color = colors::WHITE);

        virtual void render_ui() override;
        virtual void update_ui(Controller* controller) override;
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

        bool is_color_button        = false;
        bool is_submenu             = false;
        bool is_unique_selection    = false;
        bool allow_toggle           = false;
        bool allow_events           = true;

        ButtonWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c);

        virtual void render_ui() override;
		virtual void update_ui(Controller* controller) override;
	};

	class SliderWidget : public UIEntity {
	public:

        TextWidget* label = nullptr;
        Color color;
        std::string signal;

		float current_value = 0.f;

		SliderWidget(const std::string& sg, float v, const glm::vec2& p, const Color& c, const glm::vec2& s);

        virtual void render_ui() override;
		virtual void update_ui(Controller* controller) override;
	};

    class ColorPickerWidget : public UIEntity {
	public:

		Color current_color;
        std::string signal;

		ColorPickerWidget(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c);

        virtual void update_ui(Controller* controller);
	};
}
