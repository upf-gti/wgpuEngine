#pragma once

#include "framework/ui/ui_utils.h"
#include "framework/colors.h"
#include "framework/nodes/panel_2d.h"
#include "framework/nodes/container_2d.h"

#include "graphics/mesh_instance.h"

#include "text.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

    class Button2D : public Panel2D {
    public:

        Text2D* text_2d = nullptr;

        // Animations
        float target_scale = 1.0f;
        float timer = 0.0f;

        bool is_unique_selection    = false;
        bool allow_toggle           = false;
        bool selected               = false;
        bool disabled               = false;
        bool label_as_background    = false;
        bool is_color_button        = true;

        Button2D() {};
        Button2D(const std::string& sg, const Color& color = colors::WHITE, uint32_t parameter_flags = 0);
        Button2D(const std::string& sg, uint32_t parameter_flags, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));
        Button2D(const std::string& sg, const Color& color, uint32_t parameter_flags, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void set_disabled(bool value);
        void set_selected(bool value);
        void set_is_unique_selection(bool value);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;
    };

    class TextureButton2D : public Button2D {
    public:

        TextureButton2D(const std::string& sg, const std::string& texture_path, uint32_t parameter_flags = 0);
        TextureButton2D(const std::string& sg, const std::string& texture_path, uint32_t parameter_flags, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));
    };

    class ConfirmButton2D : public TextureButton2D {

        bool confirm_pending = false;
        std::string texture_path;
        float confirm_timer = 0.0f;

    public:

        ConfirmButton2D(const std::string& sg, const std::string& texture_path, uint32_t parameter_flags = 0);
        ConfirmButton2D(const std::string& sg, const std::string& texture_path, uint32_t parameter_flags, const glm::vec2& pos, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void update(float delta_time) override;
        bool on_input(sInputData data) override;
    };

    class ButtonSubmenu2D : public TextureButton2D {
    public:

        HContainer2D* box = nullptr;

        ButtonSubmenu2D(const std::string& sg, const std::string& texture_path, uint32_t parameter_flags = 0, const glm::vec2& pos = { 0.0f, 0.0f }, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void add_child(Node2D* child) override;
    };

    class ButtonSelector2D : public TextureButton2D {
    public:

        CircleContainer2D* box = nullptr;

        ButtonSelector2D(const std::string& sg, const std::string& texture_path, uint32_t parameter_flags = 0, const glm::vec2& pos = { 0.0f, 0.0f }, const glm::vec2& size = glm::vec2(BUTTON_SIZE));

        void add_child(Node2D* child) override;
        std::vector<Node*>& get_children() override;
    };

    class ComboButtons2D : public HContainer2D {
    public:

        ComboButtons2D(const std::string& name, const glm::vec2& pos = { 0.0f, 0.0f }, uint32_t flags = 0u, const Color& color = colors::GRAY);

        void on_children_changed() override;
    };
}
