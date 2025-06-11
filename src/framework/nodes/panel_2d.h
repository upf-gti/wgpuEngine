#pragma once

#include "framework/ui/ui_utils.h"
#include "framework/colors.h"
#include "graphics/mesh.h"
#include "node_2d.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

class TextEntity;

namespace ui {

    class Panel2D : public Node2D {
    public:

        Color color = glm::vec4(0.0f);

        bool render_background  = true;
        bool pressed_inside     = false;
        bool on_hover           = false;

        float last_release_time  = 0.0f;
        float last_press_time    = 0.0f;

        uint32_t parameter_flags = 0;

        Mesh* quad_mesh = nullptr;

        Panel2D() {};
        Panel2D(const std::string& name, const glm::vec2& p, const glm::vec2& s, uint32_t flags = 0u, const Color& c = colors::WHITE);
        Panel2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags, const Color& c = colors::WHITE);

        virtual ~Panel2D();

        sInputData get_input_data(bool ignore_focus = false) override;
        bool on_input(sInputData data) override;

        bool was_input_pressed();
        bool was_input_released();
        bool is_input_pressed();

        void update_scroll_view();
        bool on_pressed() override;

        void render() override;

        void disable_2d() override;
        void set_priority(uint8_t priority) override;
        void update_ui_data() override;
        void set_color(const Color& c);
        virtual void set_signal(const std::string& new_signal);

        uint32_t get_flags() { return parameter_flags; }
    };

    class Image2D : public Panel2D {
    public:

        Image2D() {};
        Image2D(const std::string& image_path, const glm::vec2& s, uint32_t flags = 0);
        Image2D(const std::string& name, const std::string& image_path, const glm::vec2& s, uint8_t priority = IMAGE);
        Image2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint8_t priority = IMAGE, uint32_t flags = 0);

        void update(float delta_time) override;
    };

    class XRPanel : public Panel2D {

        bool is_button = false;
        bool fullscreen = false;

        glm::vec2 button_position;
        glm::vec2 button_size;

    public:

        XRPanel(const std::string& name, const glm::vec2& p, const glm::vec2& s, uint32_t flags = 0, const Color& c = colors::WHITE);
        XRPanel(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags = 0, const Color& c = colors::WHITE);

        void update(float delta_time) override;

        sInputData get_input_data(bool ignore_focus = false) override;
        bool on_input(sInputData data) override;
        bool get_is_button() { return is_button; }

        void add_button(const std::string& signal, const std::string& texture_path, const glm::vec2& p, const glm::vec2& s, const Color& c = colors::WHITE);
        void make_as_button(const glm::vec2& bs, const glm::vec2& bp) { is_button = true; button_position = bp; button_size = bs; };
    };

    class Text2D : public Panel2D {

        static Text2D* selected;
        float text_scale = 1.0f;
        std::string text_string = "";

    public:

        TextEntity* text_entity = nullptr;

        Text2D() {};
        Text2D(const std::string& _text, const glm::vec2& pos, float scale = 16.f, uint32_t parameter_flags = 0, const Color& color = colors::BLACK);
        Text2D(const std::string& _text, float scale = 16.f, uint32_t parameter_flags = 0);

        void render() override;
        void update(float delta_time) override;
        bool on_input(sInputData data) override;
        void release() override;

        void set_text(const std::string& text);
        void disable_2d() override;
        void set_priority(uint8_t priority) override;
        void set_signal(const std::string& new_signal) override;

        static void select(Text2D* instance) { selected = instance; }
    };

    class ColorPicker2D : public Panel2D {
    public:

        float ring_thickness = 0.25f;

        bool changing_hue = false;
        bool changing_sv = false;

        ColorPicker2D() {};
        ColorPicker2D(const std::string& sg, const Color& c);
        ColorPicker2D(const std::string& sg, const glm::vec2& p, const glm::vec2& s, const Color& c, uint32_t flags = 0);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;

        glm::vec2 uv_to_saturation_value(const glm::vec2& uvs);
    };
}
