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

    struct sButtonDescription {
        std::string path = "";
        uint32_t flags = 0u;
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 size = glm::vec2(BUTTON_SIZE);
        Color color = colors::WHITE;
        std::string label = "";
    };

    class Button2D : public Panel2D {
    public:

        Text2D* text_2d = nullptr;

        std::string label = "";

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
        Button2D(const std::string& signal, const sButtonDescription& desc);

        void set_disabled(bool value);
        void set_selected(bool value);
        void set_is_unique_selection(bool value);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;
    };

    class TextureButton2D : public Button2D {
    public:
        TextureButton2D(const std::string& signal, const sButtonDescription& desc);
    };

    class ConfirmButton2D : public TextureButton2D {

        bool confirm_pending = false;
        std::string texture_path;
        float confirm_timer = 0.0f;
        Text2D* confirm_text_2d = nullptr;

    public:

        ConfirmButton2D(const std::string& signal, const sButtonDescription& desc);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;
    };

    class ButtonSubmenu2D : public TextureButton2D {
    public:

        HContainer2D* box = nullptr;

        ButtonSubmenu2D(const std::string& signal, const sButtonDescription& desc);

        void add_child(Node2D* child) override;
    };

    class ButtonSelector2D : public TextureButton2D {
    public:

        CircleContainer2D* box = nullptr;

        ButtonSelector2D(const std::string& signal, const sButtonDescription& desc);

        void add_child(Node2D* child) override;
        std::vector<Node*>& get_children() override;
    };

    class ComboButtons2D : public HContainer2D {
    public:

        ComboButtons2D(const std::string& name, const glm::vec2& pos = { 0.0f, 0.0f }, uint32_t flags = 0u, const Color& color = colors::GRAY);

        void on_children_changed() override;
    };
}
