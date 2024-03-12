#include "ui.h"
#include "framework/utils/utils.h"
#include "framework/ui/ui_controller.h"
#include "framework/utils/intersections.h"
#include "framework/input.h"
#include "framework/nodes/text.h"
#include "framework/camera/camera.h"
#include "graphics/renderer.h"
#include "graphics/webgpu_context.h"
#include "spdlog/spdlog.h"

namespace ui {

    /*
    *	Panel
    */

    Panel2D::Panel2D(const std::string& name, const glm::vec2& pos, const glm::vec2& size, const Color& col)
        : Node2D(name, pos, size), color(col)
    {
        class_type = Node2DClassType::PANEL;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y, false);

        quad_mesh.add_surface(quad_surface);
        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);
    }

    void Panel2D::set_color(const Color& c)
    {
        color = c;

        quad_mesh.set_surface_material_override_color(0, c);
    }

    void Panel2D::update(float delta_time)
    {
        // set_color(get_input_data() ? colors::GREEN : colors::RED);

        Node2D::update(delta_time);
    }

    void Panel2D::render()
    {
        if (!visibility)
            return;

        if (render_background) {
            // Convert the mat3x3 to mat4x4
            uint8_t priority = class_type;
            glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(get_translation(), -priority * 1e-4));
            model = glm::scale(model, glm::vec3(get_scale(), 1.0f));
            model = get_global_viewport_model() * model;
            Renderer::instance->add_renderable(&quad_mesh, model);
        }

        Node2D::render();
    }

    sInputData Panel2D::get_input_data()
    {
        sInputData data;

        WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

        Material* material = quad_mesh.get_surface_material_override(quad_mesh.get_surface(0));

        if (material->flags & MATERIAL_2D)
        {
            glm::vec2 mouse_pos = Input::get_mouse_position();
            mouse_pos.y = webgpu_context->render_height - mouse_pos.y;

            glm::vec2 min = get_translation();
            glm::vec2 max = min + size;

            data.is_hovered = mouse_pos.x >= min.x && mouse_pos.y >= min.y && mouse_pos.x <= max.x && mouse_pos.y <= max.y;
            data.is_pressed = data.is_hovered && Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
            data.was_pressed = data.is_hovered && Input::was_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
            data.was_released = Input::was_mouse_released(GLFW_MOUSE_BUTTON_LEFT);

            glm::vec2 local_mouse_pos = mouse_pos - get_translation();
            data.local_position = glm::vec2(local_mouse_pos.x, size.y - local_mouse_pos.y);
        }
        else {

            glm::vec3 ray_origin;
            glm::vec3 ray_direction;

            // HANDLE RAY USING CONTROLLER
            if (Renderer::instance->get_openxr_available())
            {
                // Ray
                ray_origin = Input::get_controller_position(HAND_RIGHT, POSE_AIM);
                glm::mat4x4 select_hand_pose = Input::get_controller_pose(HAND_RIGHT, POSE_AIM);
                ray_direction = get_front(select_hand_pose);
            }
            // HANDLE RAY USING MOUSE
            else
            {
                Camera* camera = Renderer::instance->get_camera();
                const glm::mat4x4& view_projection_inv = glm::inverse(camera->get_view_projection());

                glm::vec2 mouse_pos = Input::get_mouse_position();
                glm::vec3 mouse_pos_ndc;
                mouse_pos_ndc.x = (mouse_pos.x / webgpu_context->render_width) * 2.0f - 1.0f;
                mouse_pos_ndc.y = -((mouse_pos.y / webgpu_context->render_height) * 2.0f - 1.0f);
                mouse_pos_ndc.z = 1.0f;

                glm::vec4 ray_dir = view_projection_inv * glm::vec4(mouse_pos_ndc, 1.0f);
                ray_dir /= ray_dir.w;

                // Ray
                ray_origin = camera->get_eye();
                ray_direction = glm::normalize(glm::vec3(ray_dir));
            }

            // Quad
            uint8_t priority = class_type;
            glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(get_translation(), -priority * 1e-4));
            model = get_global_viewport_model() * model;

            glm::vec3 quad_position = model[3];
            glm::quat quad_rotation = glm::quat_cast(model);
            glm::vec2 quad_size = size * get_scale();

            float collision_dist;
            glm::vec3 intersection_point;

            data.is_hovered = intersection::ray_quad(
                ray_origin,
                ray_direction,
                quad_position,
                quad_size,
                quad_rotation,
                intersection_point,
                collision_dist,
                false
            );

            data.is_pressed = data.is_hovered && Input::is_button_pressed(XR_BUTTON_A);
            data.was_pressed = data.is_hovered && Input::was_button_pressed(XR_BUTTON_A);
            data.was_released = Input::was_button_released(XR_BUTTON_A);
            data.ray_distance = collision_dist;

            glm::vec2 local_pos = glm::vec2(intersection_point) / get_scale();
            data.local_position = glm::vec2(local_pos.x, size.y - local_pos.y);
        }

        return data;
    }

    void Panel2D::remove_flag(uint8_t flag)
    {
        Material* material = quad_mesh.get_surface_material_override(quad_mesh.get_surface(0));
        material->flags ^= flag;

        Node2D::remove_flag(flag);
    }

    /*
    *	Containers
    */

    Container2D::Container2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Panel2D(name, pos, { 0.0f, 0.0f }, col)
    {
        Material material;
        material.color = color;
        material.flags = MATERIAL_2D;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y, false);

        quad_mesh.add_surface(quad_surface);
        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        padding = glm::vec2(GROUP_MARGIN);
        item_margin = glm::vec2(GROUP_MARGIN);

        render_background = false;
    }

    void Container2D::on_children_changed()
    {
        // Recreate quad using new size and reposition accordingly

        Surface* quad_surface = quad_mesh.get_surface(0);
        quad_surface->create_quad(size.x, size.y, false);

        Node2D::on_children_changed();
    }

    HContainer2D::HContainer2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Container2D(name, pos, col)
    {
        class_type = Node2DClassType::HCONTAINER;
    }

    void HContainer2D::on_children_changed()
    {
        size_t child_count = get_children().size();
        size = glm::vec2(0.0f);

        // Get HEIGHT in first pass..
        for (size_t i = 0; i < child_count; ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            size.y = glm::max(size.y, node_2d->get_size().y);
        }

        // Reorder in WIDTH..
        for (size_t i = 0; i < child_count; ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            glm::vec2 node_size = node_2d->get_size();

            node_2d->set_translation(padding + glm::vec2(size.x + item_margin.x * static_cast<float>(i), (size.y - node_size.y) * 0.50f));

            size.x += node_size.x;
        }

        size += padding * 2.0f;
        size.x += item_margin.x * static_cast<float>(child_count - 1);

        Container2D::on_children_changed();
    }

    VContainer2D::VContainer2D(const std::string& name, const glm::vec2& pos, const Color& col)
        : Container2D(name, pos, col)
    {
        class_type = Node2DClassType::VCONTAINER;
    }

    void VContainer2D::on_children_changed()
    {
        size_t child_count = get_children().size();
        size = glm::vec2(0.0f);

        for (size_t i = 0; i < child_count; ++i)
        {
            Node2D* node_2d = static_cast<Node2D*>(get_children()[i]);
            node_2d->set_translation(padding + glm::vec2(0.0f, size.y + item_margin.y * static_cast<float>(i)));

            glm::vec2 node_size = node_2d->get_size();
            size.x = glm::max(size.x, node_size.x);
            size.y += node_size.y;
        }

        size += padding * 2.0f;
        size.y += item_margin.y * static_cast<float>(child_count - 1);

        Container2D::on_children_changed();
    }

    /*
    *   Text
    */

    Text2D::Text2D(const std::string& _text, const glm::vec2& pos, float scale, const Color& color)
        : Panel2D(_text + "@text", pos, { 1.0f, 1.0f }) {

        class_type = Node2DClassType::TEXT;

        text_entity = new TextEntity(_text);
        text_entity->set_scale(scale);
        text_entity->generate_mesh(color, MATERIAL_2D);
    }

    void Text2D::update(float delta_time)
    {
        if (!visibility)
            return;

        text_entity->set_translation(glm::vec3(get_translation(), 0.0f));

        Node2D::update(delta_time);
    }

    void Text2D::render()
    {
        if (!visibility)
            return;

        text_entity->render();

        Node2D::render();
    }

    /*
    *	Buttons
    */

    Button2D::Button2D(const std::string& sg, const Color& col, uint8_t parameter_flags)
        : Button2D(sg, col, 0, { 0.0f, 0.0f }, glm::vec2(BUTTON_SIZE)) { }

    Button2D::Button2D(const std::string& sg, uint8_t parameter_flags, const glm::vec2& pos, const glm::vec2& size)
        : Panel2D(sg, pos, size), signal(sg) { }

    Button2D::Button2D(const std::string& sg, const Color& col, uint8_t parameter_flags, const glm::vec2& pos, const glm::vec2& size)
        : Panel2D(sg, pos, size, col), signal(sg) {

        class_type = Node2DClassType::BUTTON;

        selected = parameter_flags & SELECTED;
        disabled = parameter_flags & DISABLED;
        is_unique_selection = parameter_flags & UNIQUE_SELECTION;
        allow_toggle = parameter_flags & ALLOW_TOGGLE;
        keep_rgb = parameter_flags & KEEP_RGB;

        ui_data.keep_rgb = keep_rgb;
        ui_data.is_color_button = is_color_button;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_button.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y, false);

        quad_mesh.add_surface(quad_surface);
        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 2);

        // Selection styling visibility callback..
        Node::bind(signal, [&](const std::string& signal, void* button) {

            if (disabled)
                return;

            const bool last_value = selected;

            // Unselect siblings
            if (parent && !allow_toggle) {
                for (auto c : parent->get_children()) {
                    Button2D* b = dynamic_cast<Button2D*>(c);
                    if (b) {
                        b->set_selected(false);
                    }
                }
            }
            set_selected(allow_toggle ? !last_value : true);
            });

        // Submenu icon..
        {
            /*Button2D* submenu_mark = new Button2D(sg + "@mark", {0.f, 0.f}, {0.f, 0.f}, colors::WHITE);
            submenu_mark->add_surface(RendererStorage::get_surface("quad"));

            Material material;
            material.diffuse_texture = RendererStorage::get_texture("data/textures/submenu_mark.png");
            material.flags |= MATERIAL_DIFFUSE | MATERIAL_2D;
            material.color = colors::WHITE;
            material.shader = RendererStorage::get_shader("data/shaders/ui/ui_texture.wgsl", material);

            submenu_mark->set_surface_material_override(mark->get_surface(0), material);*/
        }

        // Text label
        {
            float magic = 3.25f;
            text_2d = new Text2D(signal, { size.x * 0.5f - signal.length() * magic, 0.0f }, 16.f, colors::PURPLE);
            text_2d->set_visibility(false);
            add_child(text_2d);
        }
    }

    void Button2D::set_selected(bool value)
    {
        selected = value;

        // Only unselecting is recursive...
        if (!value)
        {
            for (auto c : children) {

                Button2D* b = dynamic_cast<Button2D*>(c);
                if (b) {
                    b->set_selected(value);
                }
            }
        }
    }

    void Button2D::render()
    {
        Panel2D::render();

        /*if (mark) mark->render();*/
    }

    void Button2D::update(float delta_time)
    {
        /*if (mark)
        {
            mark->set_model(get_model());
            mark->translate(glm::vec3(0.7f, 0.7f, -1e-3f));
            mark->scale(glm::vec3(0.35f, 0.35f, 1.0f));
        }*/

        sInputData input_data = get_input_data();

        // Check hover (intersects)
        bool hovered = input_data.is_hovered;

        text_2d->set_visibility(hovered);

        // Used to disable presses and active hovers
        hovered &= (!ui_data.is_button_disabled);

        /*
        *	Create mesh and render button
        */

        if (input_data.was_pressed)
        {
            Node::emit_signal(signal, (void*)this);

            if (selected)
            {
                if (is_color_button)
                {
                    /*if (current_selected) {
                        current_selected->selected = false;
                    }
                    current_selected = this;*/
                }
            }
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.is_selected = selected ? 1.f : 0.f;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, &quad_mesh, ui_data);

        Panel2D::update(delta_time);
    }

    TextureButton2D::TextureButton2D(const std::string& sg, const std::string& texture_path, uint8_t parameter_flags)
        : TextureButton2D(sg, texture_path, parameter_flags, { 0.0f, 0.0f }) { }

    TextureButton2D::TextureButton2D(const std::string& sg, const std::string& texture_path, uint8_t parameter_flags, const glm::vec2& pos, const glm::vec2& size)
        : Button2D(sg, parameter_flags, pos, size) {

        class_type = Node2DClassType::BUTTON;

        is_color_button = false;

        selected = parameter_flags & SELECTED;
        disabled = parameter_flags & DISABLED;
        is_unique_selection = parameter_flags & UNIQUE_SELECTION;
        allow_toggle = parameter_flags & ALLOW_TOGGLE;
        keep_rgb = parameter_flags & KEEP_RGB;

        ui_data.keep_rgb = keep_rgb;
        ui_data.is_color_button = is_color_button;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.priority = class_type;

        std::vector<std::string> define_specializations = { "USES_TEXTURE" };

        material.diffuse_texture = RendererStorage::get_texture(texture_path);
        material.flags |= MATERIAL_DIFFUSE;

        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_button.wgsl", material, define_specializations);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y, false);

        quad_mesh.add_surface(quad_surface);
        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 3);

        // Selection styling visibility callback..
        if (is_unique_selection || allow_toggle)
        {
            Node::bind(signal, [&](const std::string& signal, void* button) {

                if (disabled)
                    return;

                const bool last_value = selected;

                // Unselect siblings
                if (parent && !allow_toggle) {
                    for (auto c : parent->get_children()) {
                        Button2D* b = dynamic_cast<Button2D*>(c);
                        if (b) {
                            b->set_selected(false);
                        }
                    }
                }
                set_selected(allow_toggle ? !last_value : true);
                });
        }

        // Submenu icon..
        {
            /*Button2D* submenu_mark = new Button2D(sg + "@mark", {0.f, 0.f}, {0.f, 0.f}, colors::WHITE);
            submenu_mark->add_surface(RendererStorage::get_surface("quad"));

            Material material;
            material.diffuse_texture = RendererStorage::get_texture("data/textures/submenu_mark.png");
            material.flags |= MATERIAL_DIFFUSE | MATERIAL_2D;
            material.color = colors::WHITE;
            material.shader = RendererStorage::get_shader("data/shaders/ui/ui_texture.wgsl", material);

            submenu_mark->set_surface_material_override(mark->get_surface(0), material);*/
        }

        // Text label
        {
            float magic = 3.25f;
            text_2d = new Text2D(signal, { size.x * 0.5f - signal.length() * magic, 0.0f }, 16.f, colors::PURPLE);
            text_2d->set_visibility(false);
            add_child(text_2d);
        }
    }

    /*
    *   Widget Group
    */

    ItemGroup2D::ItemGroup2D(const std::string& name, const glm::vec2& pos, const Color& color)
        : HContainer2D(name, pos, color) {

        class_type = Node2DClassType::GROUP;

        ui_data.num_group_items = 0;

        Material material;
        material.color = color;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_group.wgsl", material);

        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 2);

        render_background = true;
    }

    float ItemGroup2D::get_number_of_items()
    {
        return ui_data.num_group_items;
    }

    void ItemGroup2D::set_number_of_items(float number)
    {
        ui_data.num_group_items = number;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, &quad_mesh, ui_data);
    }

    void ItemGroup2D::on_children_changed()
    {
        HContainer2D::on_children_changed();

        set_number_of_items(static_cast<float>(get_children().size()));
    }

    /*
    *   Widget Submenu
    */

    ButtonSubmenu2D::ButtonSubmenu2D(const std::string& sg, const std::string& texture_path, uint8_t parameter_flags, const glm::vec2& pos, const glm::vec2& size)
        : TextureButton2D(sg, texture_path, parameter_flags, pos, size) {

        class_type = Node2DClassType::SUBMENU;

        box = new ui::HContainer2D("h_container", glm::vec2(0.0f, size.y + GROUP_MARGIN));
        box->set_visibility(false);

        // box->centered = true;

        Node2D::add_child(box);

        Node::bind(sg, [&, box = box](const std::string& sg, void* data) {

            const bool last_value = box->get_visibility();

            for (auto& w : all_widgets)
            {
                ButtonSubmenu2D* submenu = dynamic_cast<ButtonSubmenu2D*>(w.second);

                if (!submenu)
                    continue;

                bool lower_layer = submenu->get_translation().y < get_translation().y;

                if (lower_layer)
                    continue;

                submenu->box->set_visibility(false);
            }

            box->set_visibility(!last_value);
            });
    }

    void ButtonSubmenu2D::add_child(Node2D* child)
    {
        box->add_child(child);
    }

    /*
    *	Slider
    */

    Slider2D::Slider2D(const std::string& sg, float v, int mode, float min, float max, float step)
        : Slider2D(sg, v, { 0.0f, 0.0f }, glm::vec2(BUTTON_SIZE), mode, min, max, step) {}

    Slider2D::Slider2D(const std::string& sg, float value, const glm::vec2& pos, const glm::vec2& size, int mode, float min, float max, float step)
        : Panel2D(sg, pos, size), signal(sg), current_value(value), min_value(min), max_value(max), step_value(step) {

        this->class_type = Node2DClassType::SLIDER;
        this->mode = mode;

        ui_data.num_group_items = mode == SliderMode::HORIZONTAL ? 2.f : 1.f;
        this->size = glm::vec2(size.x * ui_data.num_group_items, size.y);

        Material material;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_slider.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(this->size.x, this->size.y, false);

        quad_mesh.add_surface(quad_surface);
        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 2);

        Node::bind(signal + "@changed", [&](const std::string& signal, float value) {
            set_value(value);
        });

        // Text label
        {
            float magic = 3.25f;
            text_2d = new Text2D(signal, { size.x * 0.5f - signal.length() * magic, 0.0f }, 16.f, colors::PURPLE);
            add_child(text_2d);
        }
    }

    void Slider2D::update(float delta_time)
    {
        sInputData input_data = get_input_data();

        bool hovered = input_data.is_hovered;

        if (input_data.is_pressed)
        {
            float range = (mode == HORIZONTAL ? size.x : size.y);
            float bounds = range * 0.975f;
            // -scale..scale -> 0..1
            float local_point = (mode == HORIZONTAL ? input_data.local_position.x : size.y - input_data.local_position.y);
            // this is at range 0..1
            current_value = glm::clamp(local_point / bounds, 0.f, 1.f);
            // set in range min-max
            current_value = current_value * (max_value - min_value) + min_value;
            if (step_value == 1.0f) current_value = std::roundf(current_value);
            Node::emit_signal(signal, current_value);
            std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
            text_2d->text_entity->set_text(value_as_string.substr(0, 4));
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.slider_value = current_value;
        ui_data.slider_max = max_value;

        auto webgpu_context = Renderer::instance->get_webgpu_context();

        RendererStorage::update_ui_widget(webgpu_context, &quad_mesh, ui_data);

        Panel2D::update(delta_time);
    }

    void Slider2D::set_value(float new_value)
    {
        current_value = glm::clamp(new_value, min_value, max_value);
        if (step_value == 1.0f) current_value = std::roundf(current_value);
        std::string value_as_string = std::to_string(std::ceil(current_value * 100.f) / 100.f);
        text_2d->text_entity->set_text(value_as_string.substr(0, 4));
    }

    /*
    *	ColorPicker
    */

    ColorPicker2D::ColorPicker2D(const std::string& sg, const Color& c, bool skip_intensity)
        : ColorPicker2D(sg, { 0.0f, 0.0f }, glm::vec2(BUTTON_SIZE), c, skip_intensity) {}

    ColorPicker2D::ColorPicker2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size, const Color& c, bool skip_intensity)
        : Panel2D(sg, pos, size, c), signal(sg)
    {
        class_type = Node2DClassType::COLOR_PICKER;

        Material material;
        material.flags = MATERIAL_2D | MATERIAL_UI;
        material.priority = class_type;
        material.shader = RendererStorage::get_shader("data/shaders/ui/ui_color_picker.wgsl", material);

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y, false);

        quad_mesh.add_surface(quad_surface);
        quad_mesh.set_surface_material_override(quad_mesh.get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material.shader, &quad_mesh, ui_data, 2);

        if (!skip_intensity)
        {
            std::string slider_name = signal + "_intensity";
            ui::Slider2D* intensity_slider = new ui::Slider2D(slider_name, 1.0f, { size.x + GROUP_MARGIN, 0.f }, size * 0.50f, ui::SliderMode::VERTICAL);

            add_child(intensity_slider);

            Node::bind(slider_name, [&, picker_sg = sg](const std::string& _sg, float value) {
                color.a = value;
                glm::vec3 new_color = glm::pow(glm::vec3(color) * value, glm::vec3(2.2f));
                Node::emit_signal(picker_sg, Color(new_color, value));
            });

            // Set initial value
            color.a = 1.0f;
        }
    }

    void ColorPicker2D::render()
    {
        Panel2D::render();
    }

    void ColorPicker2D::update(float delta_time)
    {
        sInputData input_data = get_input_data();

        bool hovered = input_data.is_hovered;

        glm::vec2 local_mouse_pos = input_data.local_position;
        local_mouse_pos /= size;
        local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1
        float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

        // Update hover
        hovered &= (dist < 1.f);

        if (hovered && input_data.is_pressed)
        {
            constexpr float pi = glm::pi<float>();
            float r = pi / 2.f;
            local_mouse_pos = glm::mat2x2(cos(r), -sin(r), sin(r), cos(r)) * local_mouse_pos;
            glm::vec2 polar = glm::vec2(atan2(local_mouse_pos.y, local_mouse_pos.x), glm::length(local_mouse_pos));
            float percent = (polar.x + pi) / (2.0f * pi);
            glm::vec3 hsv = glm::vec3(percent, 1.0f, polar.y);

            // Store it without conversion and intensity multiplier
            color = glm::vec4(hsv2rgb(hsv), color.a);
            // Send the signal using the final color
            glm::vec3 new_color = glm::pow(glm::vec3(color) * color.a, glm::vec3(2.2f));
            Node::emit_signal(signal, glm::vec4(new_color, color.a));
        }

        if (input_data.was_released)
        {
            Node::emit_signal(signal + "@released", color);
        }

        // Update uniforms
        ui_data.is_hovered = hovered ? 1.f : 0.f;
        ui_data.picker_color = color;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, &quad_mesh, ui_data);

        Panel2D::update(delta_time);
    }

    /*
    *   Label
    */

    /*LabelWidget::LabelWidget(const std::string& p_text, const glm::vec2& p, const glm::vec2& s) : UIEntity(p, s), text(p_text) {
        type = eWidgetType::LABEL;
        center_pos = false;
    }*/
}
