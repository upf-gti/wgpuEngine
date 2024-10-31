#include "panel_2d.h"

#include <cctype>

#include "engine/engine.h"

#include "framework/math/intersections.h"
#include "framework/input.h"
#include "framework/nodes/text.h"
#include "framework/camera/camera.h"
#include "framework/utils/utils.h"
#include "framework/ui/io.h"

#include "graphics/renderer.h"
#include "graphics/webgpu_context.h"

#include "shaders/mesh_forward.wgsl.gen.h"
#include "shaders/ui/ui_xr_panel.wgsl.gen.h"
#include "shaders/ui/ui_color_picker.wgsl.gen.h"
#include "shaders/ui/ui_texture.wgsl.gen.h"
#include "shaders/ui/ui_text_shadow.wgsl.gen.h"

namespace ui {

    /*
    *	Panel
    */

    Panel2D::Panel2D(const std::string& name, const glm::vec2& pos, const glm::vec2& size, uint32_t flags, const Color& col)
        : Panel2D(name, "", pos, size, flags, col) { }

    Panel2D::Panel2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags, const Color& c)
        : Node2D(name, p, s, flags), color(c)
    {
        class_type = Node2DClassType::PANEL;

        Material* material = new Material();
        material->set_color(color);
        material->set_type(MATERIAL_UNLIT);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);

        if (image_path.size()) {
            material->set_diffuse_texture(RendererStorage::get_texture(image_path, TEXTURE_STORAGE_UI));
        }

        material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material));

        Surface* quad_surface = new Surface();
        quad_surface->create_quad(size.x, size.y, true);

        quad_mesh = new MeshInstance();
        quad_mesh->add_surface(quad_surface);
        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);
    }

    Panel2D::~Panel2D()
    {
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::delete_ui_widget(webgpu_context, quad_mesh);

        delete quad_mesh;
    }

    void Panel2D::set_color(const Color& c)
    {
        color = c;

        quad_mesh->get_surface_material_override(quad_mesh->get_surface(0))->set_color(c);
    }

    void Panel2D::set_signal(const std::string& new_signal)
    {
        all_widgets.erase(name);

        set_name(new_signal);

        all_widgets[new_signal] = this;
    }

    void Panel2D::render()
    {
        if (!visibility)
            return;

        if (render_background) {
            // Convert the mat3x3 to mat4x4
            uint8_t priority = class_type;
            glm::vec2 position = get_translation() + size * 0.5f * get_scale();
            // Don't apply the scaling to combo buttons.. 
            glm::vec2 scale = get_scale() * (class_type != Node2DClassType::COMBO_BUTTON ? scaling : glm::vec2(scaling.x, 1.0f));
            glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(position, -priority * 3e-5));
            model = glm::scale(model, glm::vec3(scale, 1.0f));
            model = get_global_viewport_model() * model;
            Renderer::instance->add_renderable(quad_mesh, model);
        }

        Node2D::render();
    }

    bool Panel2D::was_input_pressed()
    {
        Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

        bool is_2d = material->get_is_2D() || (!Renderer::instance->get_openxr_available());

        if (is_2d) {
            return Input::was_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
        }
        else {
            return Input::was_button_pressed(XR_BUTTON_A) || Input::was_trigger_pressed(HAND_RIGHT);
        }

        return false;
    }

    bool Panel2D::was_input_released()
    {
        Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

        bool is_2d = material->get_is_2D() || (!Renderer::instance->get_openxr_available());

        if (is_2d) {
            return Input::was_mouse_released(GLFW_MOUSE_BUTTON_LEFT);
        }
        else {
            return Input::was_button_released(XR_BUTTON_A) || Input::was_trigger_released(HAND_RIGHT);
        }

        return false;
    }

    bool Panel2D::is_input_pressed()
    {
        Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

        bool is_2d = material->get_is_2D() || (!Renderer::instance->get_openxr_available());

        if (is_2d) {
            return Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
        }
        else {
            return Input::is_button_pressed(XR_BUTTON_A) || Input::is_trigger_pressed(HAND_RIGHT);
        }

        return false;
    }

    sInputData Panel2D::get_input_data(bool ignore_focus)
    {
        sInputData data;

        Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

        if (material->get_is_2D())
        {
            const glm::vec2& mouse_pos = Input::get_mouse_position();
            const glm::vec2& min = get_translation();
            const glm::vec2& max = min + size;

            data.is_hovered = mouse_pos.x >= min.x && mouse_pos.y >= min.y && mouse_pos.x <= max.x && mouse_pos.y <= max.y;

            const glm::vec2& local_mouse_pos = mouse_pos - get_translation();
            data.local_position = glm::vec2(local_mouse_pos.x, size.y - local_mouse_pos.y);
        }
        else {

            glm::vec3 ray_origin;
            glm::vec3 ray_direction;

            // Handle ray using VR controller
            if (Renderer::instance->get_openxr_available()) {
                // Ray
                ray_origin = Input::get_controller_position(HAND_RIGHT, POSE_AIM);
                const glm::mat4x4& select_hand_pose = Input::get_controller_pose(HAND_RIGHT, POSE_AIM);
                ray_direction = get_front(select_hand_pose);
            }
            // Handle ray using mouse position
            else {
                Camera* camera = Renderer::instance->get_camera();
                const glm::vec3& ray_dir = camera->screen_to_ray(Input::get_mouse_position());

                // Ray
                ray_origin = camera->get_eye();
                ray_direction = glm::normalize(ray_dir);
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
            glm::vec3 local_intersection_point;

            data.is_hovered = intersection::ray_quad(
                ray_origin,
                ray_direction,
                quad_position,
                quad_size,
                quad_rotation,
                intersection_point,
                local_intersection_point,
                collision_dist,
                false
            );

            if (data.is_hovered) {
                if (Renderer::instance->get_openxr_available()) {
                    data.ray_intersection = intersection_point;
                    data.ray_distance = collision_dist;
                }

                glm::vec2 local_pos = glm::vec2(local_intersection_point) / get_scale();
                data.local_position = glm::vec2(local_pos.x, size.y - local_pos.y);
            }
        }

        data.was_pressed = data.is_hovered && was_input_pressed();

        if (data.was_pressed) {
            pressed_inside = true;
        }

        data.was_released = was_input_released();

        if (data.was_released) {
            pressed_inside = false;
        }

        data.is_pressed = pressed_inside && is_input_pressed();

        if (!on_hover && data.is_hovered) {
            data.was_hovered = true;
        }

        // Few logic for managing focus

        if (!ignore_focus) {

            if (data.was_pressed) {
                IO::set_focus(this);
            }

            data.is_pressed &= IO::equals_focus(this);
        }

        return data;
    }

    bool Panel2D::on_input(sInputData data)
    {
        if (data.was_pressed) {
            last_press_time = glfwGetTime();
        }

        if (data.is_pressed && (parameter_flags & LONG_CLICK) && (glfwGetTime() - last_press_time) > 0.5f) {
            Node::emit_signal(name + "@long_click", (void*)nullptr);
        }

        if (data.was_hovered) {
            Engine::instance->vibrate_hand(HAND_RIGHT, HOVER_HAPTIC_AMPLITUDE, HOVER_HAPTIC_DURATION);
        }

        return true;
    }

    bool Panel2D::on_pressed()
    {
        float now = glfwGetTime();

        bool skip = false;

        if ((parameter_flags & DBL_CLICK) && (now - last_release_time) < 0.4f) {
            Node::emit_signal(name + "@dbl_click", (void*)nullptr);
            skip = true;
        }

        last_release_time = now;

        return skip;
    }

    void Panel2D::update_scroll_view()
    {
        // return;

        auto parent_2d = get_parent<Node2D*>();

        if (!(parameter_flags & SCROLLABLE) || !parent_2d) {
            return;
        }

        // Last chance..
        parent_2d = parent_2d->get_parent<Node2D*>();

        if (!parent_2d || parent_2d->get_class_type() != VCONTAINER) {
            return;
        }

        ui_data.range = 1.0f;

        set_visibility(true, false);

        float size_y = get_size().y;
        float parent_size_y = parent_2d->get_size().y;
        float y = get_translation().y;
        float parent_y = parent_2d->get_translation().y;
        float y_min = parent_y;
        float y_max = parent_y + parent_size_y - size_y;
        // exceeds at the top
        if (y < y_min) {

            ui_data.range = glm::clamp((y - y_min) / size_y, -1.0f, -0.01f);

            if (class_type == TEXT_SHADOW) {
                set_visibility(false, false);
            }
        }
        // exceeds at the bottom
        else if (y > y_max) {

            ui_data.range = 1.0f - glm::clamp((y - y_max) / size_y, 0.0f, 0.99f);

            if (class_type == TEXT_SHADOW) {
                set_visibility(false, false);
            }
        }

        if (glm::abs(ui_data.range) < 0.2f) {
            set_visibility(false, false);
        }
    }

    void Panel2D::disable_2d()
    {
        Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

        material->set_is_2D(false);

        Node2D::disable_2d();
    }

    void Panel2D::set_priority(uint8_t priority)
    {
        Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));
        material->set_priority(priority);

        Node2D::set_priority(priority);
    }

    void Panel2D::update_ui_data()
    {
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::update_ui_widget(webgpu_context, quad_mesh, ui_data);
    }

    /*
    *	Image
    */

    Image2D::Image2D(const std::string& image_path, const glm::vec2& s, uint32_t flags)
        : Image2D("image_no_name", image_path, {0.0f, 0.0f}, s, IMAGE, flags) {}

    Image2D::Image2D(const std::string& name, const std::string& image_path, const glm::vec2& s, uint8_t priority)
        : Image2D(name, image_path, { 0.0f, 0.0f }, s, priority) {}

    Image2D::Image2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint8_t priority, uint32_t flags)
        : Panel2D(name, p, s)
    {
        class_type = priority;

        Material* material = new Material();
        material->set_color(color);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);
        material->set_diffuse_texture(RendererStorage::get_texture(image_path, TEXTURE_STORAGE_UI));
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_texture::source, shaders::ui_texture::path, material));

        parameter_flags = flags;

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);
    }

    void Image2D::update(float delta_time)
    {
        update_scroll_view();

        if (!visibility)
            return;

        update_ui_data();

        Node2D::update(delta_time);
    }

    /*
    *   XRPanel
    */

    XRPanel::XRPanel(const std::string& name, const Color& c, const glm::vec2& p, const glm::vec2& s, uint32_t flags)
        : XRPanel(name, "", p, s, flags, c)
    { }

    XRPanel::XRPanel(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags, const Color& c)
        : Panel2D(name, image_path, p, s, flags, c)
    {
        Surface* quad_surface = quad_mesh->get_surface(0);
        quad_surface->create_quad(size.x, size.y, true);
        // Use subdivided quad in case we use panels with curvature
        // quad_surface->create_subdivided_quad(size.x, size.y);

        parameter_flags = flags;

        Material* material = quad_mesh->get_surface_material_override(quad_surface);
        material->set_type(MATERIAL_UI);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_xr_panel::source, shaders::ui_xr_panel::path, material));
        // material->shader = RendererStorage::get_shader("data/shaders/ui_xr_panel.wgsl", *material);

        ui_data.xr_info = glm::vec4(1.0f, 1.0f, 0.5f, 0.5f);
        ui_data.aspect_ratio = size.x / size.y;

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);
    }

    sInputData XRPanel::get_input_data(bool ignore_focus)
    {
        // Use flat panel quad for flat screen..
        if (!Renderer::instance->get_openxr_available()) {
            return Panel2D::get_input_data(ignore_focus);
        }

        sInputData data;

        // Ray
        glm::vec3 ray_origin = Input::get_controller_position(HAND_RIGHT, POSE_AIM);
        glm::mat4x4 select_hand_pose = Input::get_controller_pose(HAND_RIGHT, POSE_AIM);
        glm::vec3 ray_direction = get_front(select_hand_pose);

        // Quad
        uint8_t priority = class_type;
        glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(get_translation(), -priority * 1e-4));
        model = get_global_viewport_model() * model;

        glm::vec3 quad_position = model[3];
        glm::quat quad_rotation = glm::quat_cast(model);
        glm::vec2 quad_size = size * get_scale();

        float collision_dist;
        glm::vec3 intersection_point;
        glm::vec3 local_intersection_point;

        data.is_hovered = intersection::ray_quad(
            ray_origin,
            ray_direction,
            quad_position,
            quad_size,
            quad_rotation,
            intersection_point,
            local_intersection_point,
            collision_dist,
            false
        );

        // ray_curved_quad it's not working well.. by nos using flat panels..
        /*if (data.is_hovered) {

            glm::vec2 uv = glm::vec2(local_intersection_point) * 2.0f - 1.0f;

            float z_offset = 0.15f * (1.0f - (fabsf(uv.x * uv.x) * 0.5f + 0.5f));

            intersection_point += ray_direction * z_offset;
        }*/

        data.was_pressed = data.is_hovered && was_input_pressed();

        if (data.was_pressed) {
            pressed_inside = true;
        }
        data.was_released = was_input_released();

        if (data.was_released) {
            pressed_inside = false;
        }
        data.is_pressed = pressed_inside && is_input_pressed();

        data.ray_intersection = intersection_point;
        data.ray_distance = collision_dist;

        glm::vec2 local_pos = glm::vec2(local_intersection_point) / get_scale();
        data.local_position = glm::vec2(local_pos.x, size.y - local_pos.y);

        if (!on_hover && data.is_hovered) {
            data.was_hovered = true;
        }

        return data;
    }

    void XRPanel::update(float delta_time)
    {
        if (!visibility)
            return;

        // reset event stuff..
        ui_data.hover_info.x = 0.0f;
        ui_data.hover_info.y = 0.0f;
        ui_data.press_info.x = 0.0f;

        // ignore focus process, doing it manually per button
        sInputData data = get_input_data(true);

        if (data.is_hovered) {

            if (is_button) {

                glm::vec2 local_pos = { data.local_position.x, -(data.local_position.y - size.y)};
                data.is_hovered &= local_pos.x > (button_position.x - button_size.x * 0.5f) && local_pos.x < (button_position.x + button_size.x * 0.5f);
                data.is_hovered &= local_pos.y > (button_position.y - button_size.y * 0.5f) && local_pos.y < (button_position.y + button_size.y * 0.5f);
            }

            // it's inside the button
            if (data.is_hovered) {

                if (is_button && data.was_pressed) {
                    IO::set_focus(this);
                }

                data.is_pressed &= IO::equals_focus(this);

                IO::push_input(this, data);
            }
        }

        ui_data.aspect_ratio = size.x / size.y;

        on_hover = false;

        update_ui_data();

        Node2D::update(delta_time);
    }

    bool XRPanel::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        // Don't do anything more on background xr panels..
        if (!is_button) {
            return true;
        }

        Panel2D::on_input(data);

        if (data.was_released)
        {
            if (!on_pressed()) {
                Node::emit_signal(name, (void*)this);
            }
        }

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.hover_info.y = 1.0f;
        ui_data.press_info.x = data.is_pressed ? 1.0f : 0.0f;

        on_hover = true;

        update_ui_data();

        return true;
    }

    void XRPanel::add_button(const std::string& signal, const std::string& texture_path, const glm::vec2& p, const glm::vec2& s, const Color& c)
    {
        XRPanel* new_button = new XRPanel(signal, texture_path, { 0.0f, 0.0f }, size);
        add_child(new_button);

        new_button->set_priority(PANEL_BUTTON);

        new_button->ui_data.aspect_ratio = s.x / s.y;
        new_button->ui_data.xr_info = glm::clamp(glm::vec4(s / size, p / size), 0.0f, 1.0f);

        new_button->update_ui_data();

        new_button->make_as_button(s, p);
    }

    /*
    *   Text
    */

    Text2D* Text2D::selected = nullptr;

    Text2D::Text2D(const std::string& _text, float scale, uint32_t flags)
        : Text2D(_text, { 0.0f, 0.0f }, scale, flags, colors::WHITE)
    { }

    Text2D::Text2D(const std::string& _text, const glm::vec2& pos, float scale, uint32_t flags, const Color& color)
        : Panel2D("no_signal", pos, {1.0f, 1.0f}) {

        text_string = _text;
        text_scale = scale;

        class_type = Node2DClassType::TEXT_SHADOW;
        parameter_flags = flags;

        text_entity = new TextEntity(text_string);
        text_entity->set_scale(text_scale);
        text_entity->generate_mesh(color, true);
        text_entity->get_surface_material(0)->set_priority(Node2DClassType::TEXT);

        float text_width = (float)text_entity->get_text_width(text_string);
        size.x = std::max(text_width, 24.0f) + TEXT_SHADOW_MARGIN * text_scale;
        size.y = text_scale + TEXT_SHADOW_MARGIN * text_scale * 0.5f;

        ui_data.num_group_items = size.x;

        render_background = !(flags & SKIP_TEXT_RECT);

        if ((flags & TEXT_SELECTABLE) && (flags & SELECTED)) {
            selected = this;
        }

        Material* material = new Material();
        material->set_color(colors::WHITE);
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_text_shadow::source, shaders::ui_text_shadow::path, material));

        Surface* quad_surface = quad_mesh->get_surface(0);
        quad_surface->create_quad(size.x, size.y, true);
        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        Node::bind(name + "@selected", [&](const std::string& signal, void* data) {
            if (parameter_flags & TEXT_SELECTABLE) {
                selected = this;
            }
        });

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);
    }

    void Text2D::update(float delta_time)
    {
        update_scroll_view();

        Node2D* parent = get_parent<Node2D*>();

        if ((parameter_flags & TEXT_CENTERED) && parent) {
            const glm::vec2& par_size = parent->get_size();
            const glm::vec2& centered_position = { -size.x * 0.5f + par_size.x * 0.5f, get_local_translation().y };
            set_position(centered_position);
        }

        // Set Text entity in place
        {
            uint8_t priority = class_type;
            glm::vec2 scale = get_scale();
            glm::vec2 position = get_translation() + (render_background ? glm::vec2(TEXT_SHADOW_MARGIN * text_scale, TEXT_SHADOW_MARGIN * text_scale * 0.5f) * 0.5f * scale : glm::vec2(0.0f));

            auto t = Transform::identity();
            t.translate(glm::vec3(position, -priority * 3e-5));
            t.scale(glm::vec3(scale, 1.0f));

            text_entity->set_transform(Transform::combine(Transform::mat4_to_transform(get_global_viewport_model()), t));
        }

        if (visibility && (parameter_flags & TEXT_EVENTS)) {
            sInputData data = get_input_data();

            if (data.is_hovered) {
                IO::push_input(this, data);
            }

            // Update uniforms
            ui_data.hover_info.x = 0.0f;
            ui_data.hover_info.y = 0.0f;
            ui_data.is_selected = 0.0f;

            on_hover = false;
            ui_data.is_selected = (selected == this) ? 1.0f : 0.0f;
        }

        ui_data.aspect_ratio = size.x / size.y;

        update_ui_data();

        Node2D::update(delta_time);
    }

    bool Text2D::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        Panel2D::on_input(data);

        // Internally, use on release mouse, not on press..
        if (data.was_released)
        {
            if (!on_pressed()) {
                Node::emit_signal(name, (void*)this);

                if (parameter_flags & ui::TEXT_SELECTABLE) {
                    selected = this;
                }
            }
        }

        // Send signal on move thumbstick or using wheel hovering the text
        {
            float dt = Input::get_thumbstick_value(HAND_RIGHT).y;

            if (glm::abs(dt) > 0.01f) {
                Node::emit_signal(name + "@stick_moved", dt * 0.1f);
            }

            float wheel_dt = Input::get_mouse_wheel_delta();

            if (glm::abs(wheel_dt) > 0.01f) {
                Node::emit_signal(name + "@stick_moved", wheel_dt);
            }
        }

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.hover_info.y = 1.0f;

        on_hover = true;

        update_ui_data();

        return true;
    }

    void Text2D::release()
    {
        delete text_entity;

        Node2D::release();
    }

    void Text2D::render()
    {
        if (!visibility)
            return;

        Panel2D::render();

        text_entity->render();
    }

    void Text2D::set_text(const std::string& text)
    {
        text_entity->set_text(text);

        float text_width = (float)text_entity->get_text_width(text);
        size.x = std::max(text_width, 24.0f) + TEXT_SHADOW_MARGIN * text_scale;
        size.y = text_scale + TEXT_SHADOW_MARGIN * text_scale * 0.5f;

        ui_data.num_group_items = size.x;
        update_ui_data();

        Surface* quad_surface = quad_mesh->get_surface(0);
        quad_surface->create_quad(size.x, size.y, true);

        Node2D::on_children_changed();
    }

    void Text2D::disable_2d()
    {
        Material* material = text_entity->get_surface_material(0);

        if (material->get_is_2D()) {
            material->set_is_2D(false);
            //text_entity->generate_mesh(color, material->is_2D);
        }

        Panel2D::disable_2d();
    }

    void Text2D::set_priority(uint8_t priority)
    {
        text_entity->get_surface_material(0)->set_priority(priority);

        Panel2D::set_priority(priority);
    }

    void Text2D::set_signal(const std::string& new_signal)
    {
        // Remove old events..
        Node::unbind(name + "@selected");

        Panel2D::set_signal(new_signal);

        // Bind new ones
        Node::bind(name + "@selected", [&](const std::string& signal, void* data) {
            if (parameter_flags & TEXT_SELECTABLE) {
                selected = this;
            }
        });
    }

    /*
    *	ColorPicker
    */

    ColorPicker2D::ColorPicker2D(const std::string& sg, const Color& c)
        : ColorPicker2D(sg, { 0.0f, 0.0f }, glm::vec2(PICKER_SIZE), c, 0) {}

    ColorPicker2D::ColorPicker2D(const std::string& sg, const glm::vec2& pos, const glm::vec2& size, const Color& c, uint32_t flags)
        : Panel2D(sg, pos, size, flags, c)
    {
        class_type = Node2DClassType::COLOR_PICKER;

        parameter_flags = flags;

        Material* material = new Material();
        material->set_type(MATERIAL_UI);
        material->set_is_2D(true);
        material->set_cull_type(CULL_BACK);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_priority(class_type);
        material->set_shader(RendererStorage::get_shader_from_source(shaders::ui_color_picker::source, shaders::ui_color_picker::path, material));

        color = Color(rgb2hsv(color), 1.0f);

        quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

        auto webgpu_context = Renderer::instance->get_webgpu_context();
        RendererStorage::register_ui_widget(webgpu_context, material->get_shader_ref(), quad_mesh, ui_data, 3);

        Node::bind(name + "@changed", [&](const std::string& signal, Color color) {
            glm::vec3 new_color = rgb2hsv(glm::pow(glm::vec3(color), glm::vec3(1.0f / 2.2f)));
            this->color = glm::vec4(new_color, 1.0f);
        });
    }

    void ColorPicker2D::update(float delta_time)
    {
        update_scroll_view();

        if (!visibility)
            return;

        sInputData data = get_input_data();

        bool hovered = data.is_hovered;

        glm::vec2 local_mouse_pos = data.local_position;
        local_mouse_pos /= size;
        local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1
        float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

        // Update hover
        hovered &= (dist < 1.f);

        if (hovered) {
            IO::push_input(this, data);
        }
        else {
            changing_hue = changing_sv = false;
        }

        // Update uniforms
        ui_data.hover_info.x = 0.0f;
        ui_data.picker_color = color;

        on_hover = false;

        update_ui_data();

        Node2D::update(delta_time);
    }

    bool ColorPicker2D::on_input(sInputData data)
    {
        IO::set_hover(this, data);

        glm::vec2 local_mouse_pos = data.local_position;
        local_mouse_pos /= size;
        local_mouse_pos = local_mouse_pos * 2.0f - 1.0f; // -1..1

        float dist = glm::distance(local_mouse_pos, glm::vec2(0.f));

        if (data.is_pressed) {

            // Change HUE
            if ((changing_hue && dist > 0.1f) || (dist > (1.0f - ring_thickness) && !changing_sv)) {
                color.r = fmod(glm::degrees(atan2f(local_mouse_pos.y, local_mouse_pos.x)), 360.f);
                changing_hue = true;
            }

            // Update Saturation, Value
            else if(!changing_hue){
                glm::vec2 uv = local_mouse_pos;
                glm::vec2 sv = uv_to_saturation_value(uv);
                color = { color.r, sv.x, sv.y, 1.0f };
                changing_sv = true;
            }

            // Send the signal using the final color
            glm::vec3 new_color = glm::pow(hsv2rgb(color), glm::vec3(2.2f));
            Node::emit_signal(name, glm::vec4(new_color, 1.0));

            on_pressed();
        }

        if (data.was_released) {
            Node::emit_signal(name + "@released", color);
            changing_hue = changing_sv = false;
        }

        if (data.was_hovered) {
            Engine::instance->vibrate_hand(HAND_RIGHT, HOVER_HAPTIC_AMPLITUDE, HOVER_HAPTIC_DURATION);
        }

        // Update uniforms
        ui_data.hover_info.x = 1.0f;
        ui_data.picker_color = color;

        on_hover = true;

        update_ui_data();

        return true;
    }

    glm::vec2 ColorPicker2D::uv_to_saturation_value(const glm::vec2& uvs)
    {
        float angle = glm::radians(color.r);
        glm::vec2 v1 = { cos(angle), sin(angle) };
        angle += glm::radians(120.f);
        glm::vec2 v2 = { cos(angle), sin(angle) };
        angle += glm::radians(120.f);
        glm::vec2 v3 = { cos(angle), sin(angle) };

        v1 *= (1.0f - ring_thickness);
        v2 *= (1.0f - ring_thickness);
        v3 *= (1.0f - ring_thickness);

        glm::vec2 base_s = v1 - v3;
        glm::vec2 base_v = base_s * 0.5f - (v2 - v3);
        glm::vec2 base_o = v3 - base_v;
        glm::vec2 p = uvs - base_o;
        float s = dot(p, base_s) / pow(length(base_s), 2.f);
        float v = dot(p, base_v) / pow(length(base_v), 2.f);
        s -= 0.5f;
        s /= v;
        s += 0.5f;

        return glm::clamp(glm::vec2(s, v), glm::vec2(0.f), glm::vec2(1.f));
    }
}
