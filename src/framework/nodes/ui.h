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

    const float BUTTON_SIZE = 64.f;
    const float GROUP_MARGIN = 8.f;
    const float LAYER_MARGIN = 12.f;

    class Panel2D : public Node2D {
    public:

        Color color;

        bool centered = false;

        MeshInstance3D* quad = nullptr;

        Panel2D(const std::string& name, const glm::vec2& p, const glm::vec2& s, const Color& c = colors::WHITE);

        void set_color(const Color& c);

        void update(float delta_time) override;
        void render() override;
    };

    class Container2D : public Panel2D {
    public:

        glm::vec2 padding;
        glm::vec2 item_margin;

        Container2D(const std::string& name, const glm::vec2& p, const Color& c = colors::WHITE);

        void on_children_changed() override;
    };

    class HContainer2D : public Container2D {
    public:

        HContainer2D(const std::string& name, const glm::vec2& p, const Color& c = colors::WHITE);

        void on_children_changed() override;
    };

    class VContainer2D : public Container2D {
    public:

        VContainer2D(const std::string& name, const glm::vec2& p, const Color& c = colors::WHITE);

        void on_children_changed() override;
    };

    class Text2D : public Node2D {
    public:

        TextEntity* text_entity = nullptr;

        Text2D(const std::string& _text, const glm::vec2& pos, float scale = 1.f, const Color& color = colors::WHITE);

        void set_text(const std::string& text) { text_entity->set_text(text); };

        void render() override;
    };

    enum eButtonParams : uint8_t {
        SELECTED = 1 << 0,
        DISABLED = 1 << 1,
        UNIQUE_SELECTION = 1 << 2,
        ALLOW_TOGGLE = 1 << 3,
        KEEP_RGB = 1 << 4
    };

    class Button2D : public Panel2D {
    public:

        // TextWidget* label = nullptr;

        std::string     signal;

        // ButtonWidget* mark = nullptr;

        bool is_unique_selection    = false;
        bool allow_toggle           = false;
        bool selected               = false;
        bool keep_rgb               = false;
        bool disabled               = false;
        bool is_color_button        = true;

        Button2D(const std::string& sg, const Color& color = colors::WHITE);
        Button2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));
        Button2D(const std::string& sg, const Color& color, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void set_selected(bool value);

        void update(float delta_time) override;
        void render() override;
    };

    class TextureButton2D : public Button2D {
    public:

        TextureButton2D(const std::string& sg, const std::string& texture_path, uint8_t parameter_flags = 0);
        TextureButton2D(const std::string& sg, const std::string& texture_path, uint8_t parameter_flags, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));
    };

    class ItemGroup2D : public HContainer2D {
    public:

        ItemGroup2D(const std::string& name, const glm::vec2& pos = {0.0f, 0.0f}, const Color& color = colors::GRAY);

        float get_number_of_items();
        void set_number_of_items(float number);

        void on_children_changed() override;
    };

    class ButtonSubmenu2D : public TextureButton2D {
    public:

        HContainer2D* box = nullptr;

        ButtonSubmenu2D(const std::string& sg, const std::string& texture_path, const glm::vec2& pos = { 0.0f, 0.0f }, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void add_child(Node2D* child) override;
    };

    enum SliderMode {
        HORIZONTAL,
        VERTICAL
    };

    class Slider2D : public Panel2D {
    public:

        /*TextWidget* label = nullptr;
        TextWidget* text_value = nullptr;*/

        std::string     signal;

        int mode = SliderMode::HORIZONTAL;

        float current_value = 0.0f;
        float min_value = 0.0f;
        float max_value = 1.0f;
        float step_value = 0.0f;

        Slider2D(const std::string& sg, float v, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE), int mode = SliderMode::VERTICAL, float min = 0.0f, float max = 1.0f, float step = 0.0f);

        void update(float delta_time) override;
        void render() override;

        void set_value(float new_value);
        void create_helpers();
    };

    class ColorPicker2D : public Panel2D {
    public:

        std::string     signal;

        ColorPicker2D(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c, bool skip_intensity = false);

        void update(float delta_time) override;
        void render() override;
    };

	/*
    class LabelWidget : public UIEntity {
    public:

        int button = -1;

        std::string text;
        std::string subtext;

        LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s = {0.f, 0.f});
    };*/
}
