#pragma once

#include "framework/ui/ui_utils.h"
#include "framework/colors.h"
#include "graphics/mesh_instance.h"
#include "framework/nodes/panel_2d.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

    class Container2D : public Panel2D {
    public:

        glm::vec2 fixed_size = {};
        bool use_fixed_size = false;

        bool centered = false;
        bool can_hover = false;

        glm::vec2 padding = { 0.0f, 0.0f };
        glm::vec2 item_margin = { 0.0f, 0.0f };

        Container2D() {};
        Container2D(const std::string& name, const glm::vec2& p, const glm::vec2& s = { 0.0f, 0.0f }, const Color& c = colors::WHITE);

        void update(float delta_time) override;
        bool on_input(sInputData data) override;
        void on_children_changed() override;

        void set_hoverable(bool value);
        void set_centered(bool value);
        void set_fixed_size(const glm::vec2& new_size);
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

    class ItemGroup2D : public HContainer2D {
    public:

        ItemGroup2D(const std::string& name, const glm::vec2& pos = { 0.0f, 0.0f }, const Color& color = colors::GRAY);

        float get_number_of_items();
        void set_number_of_items(float number);

        void on_children_changed() override;
    };

    class ComboButtons2D : public HContainer2D {
    public:

        ComboButtons2D(const std::string& name, const glm::vec2& pos = { 0.0f, 0.0f }, const Color& color = colors::GRAY);

        void on_children_changed() override;
    };

    class ImageLabel2D : public HContainer2D {

        uint8_t mask = 0;

    public:

        Text2D* text = nullptr;

        ImageLabel2D(const std::string& p_text, const std::string& image_path, uint8_t mask = 0, const glm::vec2& scale = { 1.f, 1.f }, float text_scale = 16.0f, const glm::vec2& p = { 0.f, 0.f });

        void set_text(const std::string& p_text) { text->set_text(p_text); }

        uint8_t get_mask() { return mask; }
    };
}
