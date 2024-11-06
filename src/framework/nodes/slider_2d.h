#pragma once

#include "framework/colors.h"
#include "graphics/mesh_instance.h"
#include "framework/nodes/panel_2d.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

    enum SliderMode {
        HORIZONTAL,
        VERTICAL
    };

    class Slider2D : public Panel2D {
    protected:

        Text2D* text_2d = nullptr;
        Text2D* text_2d_value = nullptr;

        int mode = SliderMode::VERTICAL;

        bool disabled = false;

        float hover_factor = 0.0f;
        float target_hover_factor = 0.0f;
        float timer = 0.0f;

        virtual std::string value_to_string() = 0;

    public:

        Slider2D() {};
        Slider2D(const std::string& name, const glm::vec2& p, const glm::vec2& s, uint32_t flags = 0);

        void set_disabled(bool new_disabled);

        template <typename T>
        void process_wheel_joystick(T wheel_multiplier, T joystick_multiplier);
    };

    class FloatSlider2D : public Slider2D {

        float original_value = 0.0f;
        float current_value = 0.0f;
        float min_value = 0.0f;
        float max_value = 1.0f;

        int precision = 1;

    public:

        FloatSlider2D() {};
        FloatSlider2D(const std::string& sg, float v, int mode = SliderMode::VERTICAL, uint32_t flags = 0, float min = 0.0f, float max = 1.0f, int precision = 1);
        FloatSlider2D(const std::string& sg, const std::string& texture_path, float v, int mode = SliderMode::VERTICAL, uint32_t flags = 0, float min = 0.0f, float max = 1.0f, int precision = 1);
        FloatSlider2D(const std::string& sg, const std::string& texture_path, float v, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE),
            int mode = SliderMode::VERTICAL, uint32_t flags = 0, float min = 0.0f, float max = 1.0f, int precision = 1);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;

        void set_value(float new_value);

        std::string value_to_string() override;
    };

    class IntSlider2D : public Slider2D {

        int original_value = 0;
        int current_value = 0;
        int min_value = -10;
        int max_value = 10;

    public:

        IntSlider2D() {};
        IntSlider2D(const std::string& sg, int v, int mode = SliderMode::VERTICAL, uint32_t flags = 0, int min = -10, int max = 10);
        IntSlider2D(const std::string& sg, const std::string& texture_path, int v, int mode = SliderMode::VERTICAL, uint32_t flags = 0, int min = -10, int max = 10);
        IntSlider2D(const std::string& sg, const std::string& texture_path, int v, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE),
            int mode = SliderMode::VERTICAL, uint32_t flags = 0, int min = -10, int max = 10);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;

        void set_value(int new_value);

        std::string value_to_string() override;
    };
}
