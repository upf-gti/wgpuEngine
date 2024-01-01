#include "ui_controller.h"
#include "utils.h"
#include "framework/input.h"
#include "framework/entities/entity_ui.h"
#include "framework/intersections.h"
#include "framework/entities/entity_text.h"
#include "framework/scene/parse_scene.h"

#include "spdlog/spdlog.h"

namespace ui {

    std::map<std::string, UIEntity*> Controller::all_widgets;

    float current_number_of_group_widgets; // store to make sure everything went well

	void Controller::set_workspace(glm::vec2 _workspace_size, uint8_t _select_button, uint8_t _root_pose, uint8_t _hand, uint8_t _select_hand)
	{
		global_scale = 0.001f;

		workspace = {
			.size = _workspace_size * global_scale,
			.select_button = _select_button,
			.root_pose = _root_pose,
			.hand = _hand,
			.select_hand = _select_hand
		};

        root = new UIEntity();
        ((Entity*)root)->set_process_children(true);

        raycast_pointer = parse_mesh("data/meshes/raycast.obj");

        // render_background = true;

        // Debug
        if (render_background)
        {
            background = new EntityMesh();
            background->add_surface({ RendererStorage::get_mesh("quad"), {}, background });
            background->set_material_shader(0, RendererStorage::get_shader("data/shaders/mesh_color.wgsl"));
            background->set_material_color(0, colors::RED);
        }
	}

	bool Controller::is_active()
	{
        return workspace.hand == HAND_RIGHT || Input::is_button_pressed(XR_BUTTON_X);
	}

	void Controller::render()
	{
		if (!enabled || !is_active()) return;

        if (render_background) background->render();

        root->render();

        if(workspace.hand == HAND_LEFT)
		    raycast_pointer->render();
	}

	void Controller::update(float delta_time)
	{
		if (!enabled || !is_active()) return;

		uint8_t hand = workspace.hand;
		uint8_t pose = workspace.root_pose;

		// Update raycast helper

        if (hand == HAND_LEFT)
        {
		    glm::mat4x4 raycast_transform = Input::get_controller_pose(workspace.select_hand, pose);
		    raycast_pointer->set_model(raycast_transform);
		    raycast_pointer->rotate(glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
		    raycast_pointer->scale(glm::vec3(0.1f));
        }

		// Update workspace

        global_transform = Input::get_controller_pose(hand, pose);
        global_transform = glm::rotate(global_transform, glm::radians(120.f), glm::vec3(1.f, 0.f, 0.f));

        if (pose == POSE_GRIP)
            global_transform = glm::rotate(global_transform, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));

        if (render_background)
        {
            background->set_model(global_transform);
            background->translate(glm::vec3(0.f, 0.f, 1e-3f));
            background->scale(glm::vec3(workspace.size.x, workspace.size.y, 1.f));
            
        }

		// Update widgets using this controller (not the root!)

        for (auto child : root->get_children()) {
            child->update(delta_time);
        }

        // Update controller buttons

        for (auto& it : controller_signals)
        {
            if (!Input::was_button_pressed(it.first))
                continue;

            for (auto& callback : it.second)
                callback();
        }
	}

	void Controller::process_params(glm::vec2& position, glm::vec2& size, bool skip_to_local)
	{
		position *= global_scale;
		size *= global_scale;

		// Clamp to workspace limits
		size = glm::clamp(size, glm::vec2(0.f), workspace.size - position);

		// To workspace local size
		if(!skip_to_local)
			position -= (workspace.size - size - position);
	}

    // Gets next button position (applies margin)
    glm::vec2 Controller::compute_position(float xOffset)
    {
        float x, y;

        if (group_opened)
        {
            x = last_layout_pos.x + (g_iterator * BUTTON_SIZE + g_iterator * X_GROUP_MARGIN);
            y = last_layout_pos.y;
            g_iterator += xOffset;
        }
        else
        {
            x = layout_iterator.x;
            y = layout_iterator.y * BUTTON_SIZE + (layout_iterator.y + 1.f) * Y_MARGIN;
            layout_iterator.x += (BUTTON_SIZE * xOffset + X_MARGIN);
            last_layout_pos = { x, y };
        }

        return { x, y };
    }

	void Controller::append_widget(UIEntity* widget, const std::string& name, UIEntity* force_parent)
	{
        widget->controller = this;

        if(force_parent)
        {
            force_parent->add_child(widget);
        }
        else if (parent_queue.size())
        {
            UIEntity* active_submenu = parent_queue.back();
			active_submenu->add_child(widget);
        }
		else
		{
			root->add_child(widget);
		}

        widget->set_name(name);
        widgets[name] = widget;
        all_widgets[name] = widget;
	}

    UIEntity* Controller::make_rect(glm::vec2 pos, glm::vec2 size, const Color& color)
	{
		process_params(pos, size);

		// Render quad in local workspace position
        UIEntity* rect = new UIEntity(pos, size);
        rect->add_surface({ RendererStorage::get_mesh("quad"), {}, rect });
		rect->set_material_shader(0, RendererStorage::get_shader("data/shaders/mesh_color.wgsl"));
        rect->set_material_color(0, color);
		append_widget(rect, "ui_rect");
		return rect;
	}

    UIEntity* Controller::make_text(const std::string& text, const std::string& alias, glm::vec2 pos, const Color& color, float scale, glm::vec2 size)
	{
		process_params(pos, size);
		scale *= global_scale;

        TextWidget* text_widget = new TextWidget(text, pos, scale, color);
		append_widget(text_widget, alias);
		return text_widget;
	}

    UIEntity* Controller::make_label(const json& j)
    {
        std::string text = j["name"];
        std::string alias = j.value("alias", text);

        static int num_labels = 0;

        // World attributes
        float workspace_width = workspace.size.x / global_scale;
        float margin = 2.f;

        // Text follows after icon (right)
        glm::vec2 pos = { LABEL_BUTTON_SIZE + 4.f, num_labels * LABEL_BUTTON_SIZE + (num_labels + 1.f) * margin };
        glm::vec2 size = glm::vec2(workspace_width, LABEL_BUTTON_SIZE);

        UIEntity* text_widget = make_text(text, "text@" + alias, pos, colors::WHITE, 12.f);
        text_widget->m_priority = -1;
        ((Entity*)text_widget)->set_process_children(true);
        text_widget->center_pos = false;

        // Icon goes to the left of the workspace
        pos = { 
            -workspace_width * 0.5f + LABEL_BUTTON_SIZE * 0.5f,
            num_labels * LABEL_BUTTON_SIZE + (num_labels + 1.f) * margin
        };

        process_params(pos, size);

        // Icon 
        LabelWidget* m_icon = new LabelWidget(text, pos, glm::vec2(size.y, size.y));
        m_icon->add_surface({ RendererStorage::get_mesh("quad"), {}, m_icon });

        if (j.count("texture") > 0)
        {
            std::string texture = j["texture"];
            m_icon->set_material_diffuse(0, RendererStorage::get_texture(texture));
        }

        m_icon->set_material_shader(0, RendererStorage::get_shader("data/shaders/mesh_texture.wgsl"));

        m_icon->button = j.value("button", -1);
        m_icon->subtext = j.value("subtext", "");

        if (m_icon->button != -1)
        {
            bind(m_icon->button, [widget = m_icon, text_widget = text_widget]() {
                widget->selected = !widget->selected;
                static_cast<ui::TextWidget*>(text_widget)->set_text(widget->selected ? widget->subtext : widget->text);
            });
        }

        append_widget(m_icon, alias, text_widget);

        num_labels++;

        return m_icon;
    }

    UIEntity* Controller::make_button(const json& j)
	{
        std::string signal = j["name"];

        // World attributes
        glm::vec2 pos = compute_position();
        glm::vec2 size = glm::vec2(BUTTON_SIZE);

		glm::vec2 _pos = pos;
		glm::vec2 _size = size;

		process_params(pos, size);

		/*
		*	Create button entity and set transform
		*/

        std::string texture = j["texture"];
        std::string shader = "data/shaders/ui/ui_button.wgsl";

        if (j.count("shader"))
            shader = j["shader"];

        const bool allow_toggle = j.value("allow_toggle", false);
        const bool is_color_button = j.count("color") > 0;
        Color color = is_color_button ? load_vec4(j["color"]) : colors::WHITE;

		// Render quad in local workspace position
        ButtonWidget* e_button = new ButtonWidget(signal, pos, size, color);
        e_button->add_surface({ RendererStorage::get_mesh("quad"), {}, e_button });
		e_button->set_material_shader(0, RendererStorage::get_shader(shader));
		e_button->set_material_diffuse(0, RendererStorage::get_texture(texture));
        e_button->set_material_flag(0, MATERIAL_UI);
        e_button->set_material_color(0, color);

        // Widget props
        e_button->is_color_button = is_color_button;
        e_button->is_unique_selection = j.value("unique_selection", false);
        e_button->selected = j.value("selected", false);
        e_button->ui_data.keep_rgb = j.value("keep_rgb", false) ? 1.f : 0.f;

        if( group_opened )
            e_button->m_priority = 1;

        if (is_color_button || e_button->is_unique_selection || allow_toggle)
        {
            bind(signal, [widget = e_button, allow_toggle](const std::string& signal, void* button) {
                // Unselect siblings
                UIEntity* parent = static_cast<ui::UIEntity*>(widget->get_parent());
                const bool last_value = widget->selected;
                if (!allow_toggle)
                {
                    for (auto w : parent->get_children())
                        static_cast<ui::UIEntity*>(w)->set_selected(false);
                }
                widget->set_selected(allow_toggle ? !last_value : true);
            });
        }

        e_button->m_layer = static_cast<uint8_t>(layout_iterator.y);

		append_widget(e_button, signal);
		return e_button;
	}

    UIEntity* Controller::make_slider(const json& j)
	{
        std::string signal = j["name"];
        std::string mode = j.value("mode", "horizontal");

        // World attributes
        float offset = (mode == "horizontal" ? 2.f : 1.f);
        glm::vec2 pos = compute_position( offset );
        glm::vec2 size = glm::vec2(BUTTON_SIZE * offset, BUTTON_SIZE); // Slider space is 2*BUTTONSIZE at X

        glm::vec2 _pos = pos;
        glm::vec2 _size = size;

        process_params(pos, size);

		/*
		*	Create slider entity
		*/

        float default_value = j.value("default", 1.f);
        Color color = Color(0.47f, 0.37f, 0.94f, 1.f);
        if (j.count("color"))
            color = load_vec4(j["color"]);

		SliderWidget* slider = new SliderWidget(signal, default_value, pos, size, color);
        slider->add_surface({ RendererStorage::get_mesh("quad"), {}, slider });
        slider->set_material_shader(0,RendererStorage::get_shader("data/shaders/ui/ui_slider.wgsl"));
        slider->set_material_diffuse(0, RendererStorage::get_texture(
            (mode == "horizontal" ? "data/textures/slider.png" : "data/textures/circle_white.png")));
        slider->set_material_color(0, color);
        slider->set_material_flag(0, MATERIAL_UI);
        slider->set_mode(mode);
        slider->min_value = j.value("min", slider->min_value);
        slider->max_value = j.value("max", slider->max_value);
        ((Entity*)slider)->set_process_children(true);
        slider->ui_data.num_group_items = offset;
        slider->m_layer = static_cast<uint8_t>(layout_iterator.y);

        if (group_opened)
            slider->m_priority = 1;

		append_widget(slider, signal);

		return slider;
	}

    UIEntity* Controller::make_color_picker(const json& j)
    {
        std::string signal = j["name"];
        bool has_slider = j.count("slider") > 0.f;

        // World attributes
        glm::vec2 pos = compute_position();
        glm::vec2 size = glm::vec2(BUTTON_SIZE);

        glm::vec2 _pos = pos;
        glm::vec2 _size = size;

        process_params(pos, size);

        Color default_color = load_vec4(j.value("default", ""));

        ColorPickerWidget* picker = new ColorPickerWidget(signal, pos, size, default_color);
        picker->add_surface({ RendererStorage::get_mesh("quad"), {}, picker });
        picker->set_material_shader(0,RendererStorage::get_shader("data/shaders/ui/ui_color_picker.wgsl"));
        picker->set_material_diffuse(0, RendererStorage::get_texture("data/textures/circle_white.png"));
        picker->set_material_color(0, Color(0.175f));
        picker->set_material_flag(0, MATERIAL_UI);
        picker->m_layer = static_cast<uint8_t>(layout_iterator.y);
        ((Entity*)picker)->set_process_children(true);

        if (group_opened)
            picker->m_priority = 1;

        append_widget(picker, signal);

        if (has_slider)
        {
            // Vertical slider
            SliderWidget* slider = (SliderWidget*)make_slider( j["slider"] );
            bind(j["slider"].value("name", ""), [this, p = picker](const std::string& signal, float value) {
                p->current_color.a = value;
                emit_signal(p->signal, p->current_color * value);
            });

            // Set initial value
            picker->current_color.a = j["slider"].value("default", 1.f);
        }

        return picker;
    }

	void Controller::make_submenu(ui::UIEntity* widget, const std::string& name)
	{
        static_cast<ButtonWidget*>(widget)->is_submenu = true;

        // Add mark

        {
            ButtonWidget* mark = new ButtonWidget(widget->get_name() + "@mark", {0.f, 0.f}, {0.f, 0.f}, colors::WHITE);
            mark->add_surface({ RendererStorage::get_mesh("quad"), {}, mark });
            mark->set_material_shader(0, RendererStorage::get_shader("data/shaders/ui/ui_button.wgsl"));
            mark->set_material_diffuse(0, RendererStorage::get_texture("data/textures/submenu_mark.png"));
            mark->set_material_flag(0, MATERIAL_UI);
            mark->set_material_color(0, colors::WHITE);
            ((ButtonWidget*)widget)->mark = mark;
        }

        // Visibility callback...
		bind(name, [widget = widget](const std::string& signal, void* button) {

            const bool last_value = widget->get_process_children();

            for (auto& w : all_widgets)
            {
                ButtonWidget* b = dynamic_cast<ButtonWidget*>(w.second);

                if (!b || !b->is_submenu || b->m_layer < widget->m_layer)
                    continue;

                b->set_process_children(false);
            }

            widget->set_process_children(!last_value);
		});

        // root layer width
        if (layers_width[0] == 0.f)
            layers_width[0] = layout_iterator.x;

        // Set new interators
        layout_iterator.x = 0.f;
        layout_iterator.y = widget->m_layer + 1.f;

        // Update last layout pos
        float x = 0.f;
        float y = layout_iterator.y * BUTTON_SIZE + (layout_iterator.y + 1.f) * Y_MARGIN;
        last_layout_pos = { x, y };

        // Set as new parent...
        parent_queue.push_back(widget);
	}

    void Controller::close_submenu()
    {
        UIEntity* active_submenu = parent_queue.back();

        // Store layer width to center widgets
        layers_width[active_submenu->uid] = layout_iterator.x;

        // Remove parent...
        parent_queue.pop_back();
    }

    UIEntity* Controller::make_group(const std::string& group_name, float number_of_widgets, const Color& color)
    {
        current_number_of_group_widgets = number_of_widgets;

        layout_iterator.x += X_MARGIN;

        // World attributes
        glm::vec2 pos = compute_position() - X_MARGIN;
        glm::vec2 size = glm::vec2(
            BUTTON_SIZE * number_of_widgets + (number_of_widgets - 1.f) * X_GROUP_MARGIN + X_MARGIN * 2.f,
            BUTTON_SIZE + X_MARGIN * 2.f
        );

        glm::vec2 _size = size;

        process_params(pos, size);

        WidgetGroup* group = new WidgetGroup(pos, size, number_of_widgets);
        group->add_surface({ RendererStorage::get_mesh("quad"), {}, group });
        group->set_material_shader(0, RendererStorage::get_shader("data/shaders/ui/ui_group.wgsl"));
        group->set_material_color(0, color);
        group->set_material_flag(0, MATERIAL_UI);
        group->set_layer( static_cast<uint8_t>(layout_iterator.y) );

        append_widget(group, group_name);

        parent_queue.push_back(group);
        group_opened = true;
        layout_iterator.x += _size.x - (BUTTON_SIZE + X_GROUP_MARGIN);

        return group;
    }

    void Controller::close_group()
    {
        assert(g_iterator == current_number_of_group_widgets && "Num Widgets in group does not correspond");

        layout_iterator.x -= X_GROUP_MARGIN;

        // Clear group info
        parent_queue.pop_back();
        group_opened = false; 
        g_iterator = 0.f;
    }

	void Controller::bind(const std::string& name, SignalType callback)
	{
        mapping_signals[name].push_back(callback);
	}

    void Controller::bind(uint8_t button, FuncEmpty callback)
    {
        controller_signals[button].push_back(callback);
    }

    UIEntity* Controller::get(const std::string& alias)
    {
        if (all_widgets.count(alias)) return all_widgets[alias];
        return nullptr;
    }

    UIEntity* Controller::get_widget_from_name(const std::string& alias)
    {
        if(widgets.count(alias)) return widgets[alias];
        return nullptr;
    }

    float Controller::get_layer_width(unsigned int uid)
    {
        UIEntity* widget = nullptr;

        for (auto& it : all_widgets) {

            if (it.second->uid == uid)
            {
                widget = it.second;
                break;
            }
        }

        if (!widget)
            return 0.f;

        UIEntity* parent = static_cast<ui::UIEntity*>(widget->get_parent());
        if (parent == nullptr) return 0.f;

        // Go up to get root or submenu
        while (parent->uid != 0 && !parent->is_submenu)
            parent = static_cast<ui::UIEntity*>(parent->get_parent());

        return (layers_width[parent->uid] - X_MARGIN) * global_scale;
    }

    void Controller::load_layout(const std::string& filename)
    {
        const json& j = load_json(filename);
        mjson = j;
        float group_elements_pending = -1;

        float width = j["width"];
        float height = j["height"];
        set_workspace({ width, height });

        std::function<void(const json&)> read_element = [&](const json& j) {

            std::string name = j["name"];
            std::string type = j["type"];

            if (type == "group")
            {
                assert(j.count("nitems") > 0);
                float nitems = j["nitems"];
                group_elements_pending = nitems;

                Color color = colors::GRAY;
                if (j.count("color")) {
                    color = load_vec4(j["color"]);
                }

                UIEntity* group = make_group(name, nitems, color);
            }
            else if (type == "button")
            {
                make_button(j);

                group_elements_pending--;

                if (group_elements_pending == 0.f) {
                    close_group();
                    group_elements_pending = -1;
                }
            }
            else if (type == "picker")
            {
                make_color_picker(j);

                int slots = j.count("slider") > 0 ? 2 : 1;
                group_elements_pending -= slots;

                if (group_elements_pending == 0.f) {
                    close_group();
                    group_elements_pending = -1;
                }
            }
            else if (type == "label")
            {
                make_label(j);
            }
            else if (type == "slider")
            {
                make_slider(j);

                int slots = j.value("mode", "horizontal") == "horizontal" ? 2 : 1;
                group_elements_pending -= slots;

                if (group_elements_pending == 0.f) {
                    close_group();
                    group_elements_pending = -1;
                }
            }
            else if (type == "submenu")
            {
                UIEntity* parent = get_widget_from_name(name);

                if (!parent) {
                    assert(0);
                    spdlog::error("Cannot find parent button with name {}", name);
                    return;
                }

                make_submenu(parent, name);

                assert(j.count("children") > 0);
                auto& _subelements = j["children"];
                for (auto& sub_el : _subelements) {
                    read_element(sub_el);
                }

                close_submenu();
            }
            };

        auto& _elements = j["elements"];
        for (auto& el : _elements) {
            read_element(el);
        }

        // root layer width
        if (layers_width.size() == 0)
            layers_width[0] = layout_iterator.x;
    }

    void Controller::change_list_layout(const std::string& list_name)
    {
        if (mjson.count("lists") == 0) {
            spdlog::error("Controller doesn't have layout lists...");
            return;
        }

        // Disable all widgets
        for (auto& w : widgets) {
            w.second->set_active(false);
        }

        // Enable only widgets in list...
        const json& lists = mjson["lists"];
        if (lists.count(list_name) == 0) {
            spdlog::error("Controller doesn't have a layout list named '{}'", list_name);
            return;
        }

        for (auto& it : lists[list_name]) {
            const std::string& name = it;
            auto widget = get_widget_from_name(name);
            widget->set_active(true);

            // Display also its text...
            if (widget->type == LABEL)
            {
                widget = get_widget_from_name("text@" + name);
                widget->set_active(true);
            }
        }
    }
}
