#pragma once

#include "framework/colors.h"
#include "graphics/mesh_instance.h"
#include "node_2d.h"
#include "text.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

    const float PICKER_SIZE         = 164.f;
    const float BUTTON_SIZE         = 64.f;
    const float GROUP_MARGIN        = 12.f;
    const float LAYER_MARGIN        = 8.f;
    const float TEXT_SHADOW_MARGIN  = 1.f;

    enum ComboIndex {
        UNIQUE,
        FIRST,
        MIDDLE,
        LAST
    };

    class Panel2D : public Node2D {
    public:

        static Node2D* focused;

        Color color = glm::vec4(0.0f);

        bool render_background  = true;
        bool pressed_inside     = false;

        MeshInstance quad_mesh;

        Panel2D() {};
        Panel2D(const std::string& name, const glm::vec2& p, const glm::vec2& s, const Color& c = colors::WHITE);

        sInputData get_input_data() override;

        void render() override;

        void remove_flag(uint8_t flag) override;
        void set_priority(uint8_t priority) override;
        void update_ui_data() override;
        void set_color(const Color& c);
    };

    class Container2D : public Panel2D {
    public:

        bool centered = false;

        glm::vec2 padding = { 0.0f, 0.0f };
        glm::vec2 item_margin = { 0.0f, 0.0f };

        Container2D() {};
        Container2D(const std::string& name, const glm::vec2& p, const Color& c = colors::WHITE);

        void on_children_changed() override;

        void set_centered(bool value);
    };

    class HContainer2D : public Container2D {
    public:
        HContainer2D() {};
        HContainer2D(const std::string& name, const glm::vec2& p, const Color& c = colors::WHITE);

        void on_children_changed() override;
    };

    class VContainer2D : public Container2D {
    public:
        VContainer2D() {};
        VContainer2D(const std::string& name, const glm::vec2& p, const Color& c = colors::WHITE);

        void on_children_changed() override;
    };

    class CircleContainer2D : public Container2D {
    public:
        CircleContainer2D() {};
        CircleContainer2D(const std::string& name, const glm::vec2& p, const Color& c = colors::GRAY);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;
        void on_children_changed() override;
    };

    class Text2D : public Panel2D {

        float text_scale = 1.0f;
        std::string text_string = "";

    public:

        TextEntity* text_entity = nullptr;

        Text2D() {};
        Text2D(const std::string& _text, const glm::vec2& pos, float scale = 16.f, const Color& color = colors::WHITE);
        Text2D(const std::string& _text, float scale = 16.f, bool center_big = false);

        void set_text(const std::string& text) { text_entity->set_text(text); };

        void update(float delta_time) override;
        void render() override;
        void remove_flag(uint8_t flag) override;
        void set_priority(uint8_t priority) override;
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

        Text2D* text_2d = nullptr;
        std::string signal;

        // Animations

        float target_scale = 1.0f;
        float timer = 0.0f;

        bool is_unique_selection    = false;
        bool allow_toggle           = false;
        bool selected               = false;
        bool keep_rgb               = false;
        bool disabled               = false;
        bool is_color_button        = true;

        Button2D() {};
        Button2D(const std::string& sg, const Color& color = colors::WHITE, uint8_t parameter_flags = 0);
        Button2D(const std::string& sg, uint8_t parameter_flags, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));
        Button2D(const std::string& sg, const Color& color, uint8_t parameter_flags, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void set_disabled(bool value);
        void set_selected(bool value);
        void set_is_unique_selection(bool value);

        void update(float delta_time) override;
        void on_pressed() override;
        bool on_input(sInputData data) override;
        void set_priority(uint8_t priority) override;
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

    class ComboButtons2D : public HContainer2D {
    public:

        ComboButtons2D(const std::string& name, const glm::vec2& pos = { 0.0f, 0.0f }, const Color& color = colors::GRAY);

        void on_children_changed() override;
    };

    class ButtonSubmenu2D : public TextureButton2D {
    public:

        HContainer2D* box = nullptr;

        TextureButton2D* submenu_mark = nullptr;

        ButtonSubmenu2D(const std::string& sg, const std::string& texture_path, uint8_t parameter_flags = 0, const glm::vec2& pos = { 0.0f, 0.0f }, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void render() override;
        void update(float delta_time) override;
        void add_child(Node2D* child) override;
    };

    class ButtonSelector2D : public TextureButton2D {
    public:

        CircleContainer2D* box = nullptr;

        ButtonSelector2D(const std::string& sg, const std::string& texture_path, uint8_t parameter_flags = 0, const glm::vec2& pos = { 0.0f, 0.0f }, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void add_child(Node2D* child) override;
        std::vector<Node*>& get_children() override { return box->get_children(); }
    };

    enum SliderMode {
        HORIZONTAL,
        VERTICAL
    };

    class Slider2D : public Panel2D {
    public:

        Text2D* text_2d = nullptr;

        Text2D* text_2d_value = nullptr;

        std::string signal;

        int mode = SliderMode::VERTICAL;

        bool disabled = false;

        float current_value = 0.0f;
        float min_value = 0.0f;
        float max_value = 1.0f;
        float step_value = 0.0f;

        Slider2D() {};
        Slider2D(const std::string& sg, float v, int mode = SliderMode::VERTICAL, uint8_t parameter_flags = 0, float min = 0.0f, float max = 1.0f, float step = 0.0f);
        Slider2D(const std::string& sg, const std::string& texture_path, float v, int mode = SliderMode::VERTICAL, uint8_t parameter_flags = 0, float min = 0.0f, float max = 1.0f, float step = 0.0f);
        Slider2D(const std::string& sg, const std::string& texture_path, float v, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE), int mode = SliderMode::VERTICAL, uint8_t parameter_flags = 0, float min = 0.0f, float max = 1.0f, float step = 0.0f);

        void render() override;
        void update(float delta_time) override;
        bool on_input(sInputData data) override;

        void set_value(float new_value);
    };

    class ColorPicker2D : public Panel2D {
    public:

        float ring_thickness = 0.25f;

        bool changing_hue = false;
        bool changing_sv = false;

        std::string signal;

        ColorPicker2D() {};
        ColorPicker2D(const std::string& sg, const Color& c);
        ColorPicker2D(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;

        glm::vec2 uv_to_saturation_value(const glm::vec2& uvs);
    };

    class ImageLabel2D : public HContainer2D {
        Text2D* text = nullptr;
    public:
        void set_text(const std::string& p_text) {
            text->set_text(p_text);
        }

        ImageLabel2D(const std::string& p_text, const std::string& image_path, float text_scale = 16.0f, const glm::vec2& p = { 0.f, 0.f });
    };
}
