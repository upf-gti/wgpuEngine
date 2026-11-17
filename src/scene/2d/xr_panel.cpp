#include "xr_panel.h"

#include "core/managers/input/input_manager.h"
#include "core/managers/simulation/simulation_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "scene/3d/camera_3d.h"

#include "framework/math/intersections.h"
#include "framework/ui/io.h"

#include "shaders/ui/ui_xr_panel.wgsl.gen.h"

XRPanel::XRPanel(const std::string& name, const glm::vec2& p, const glm::vec2& s, uint32_t flags, const Color& c) :
        XRPanel(name, "", p, s, flags, c)
{
}

XRPanel::XRPanel(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags, const Color& c) :
        Panel2D(name, image_path, p, s, flags, c)
{
    Surface* quad_surface = quad_mesh->get_surface(0);

    // Use subdivided quad in case we use panels with curvature
    if (XR_MANAGER->is_xr_available() && (flags & CURVED_PANEL)) {
        quad_surface->create_subdivided_quad(size.x, size.y, true);
    } else {
        quad_surface->create_quad(size.x, size.y, true);
    }

    Material* material = quad_mesh->get_surface_material_override(quad_surface);
    material->set_type(MATERIAL_UI);
    material->set_depth_read_write(false);
    material->set_shader(RenderStorage::get_singleton()->get_singleton()->get_shader_from_source(shaders::ui_xr_panel::source, shaders::ui_xr_panel::path, shaders::ui_xr_panel::libraries, material));

    // Fullscreen in 2d mode only!
    fullscreen = !XR_MANAGER->is_xr_available() && (flags & Node2DFlags::FULLSCREEN);

    ui_data.xr_size = glm::vec2(1.0f, 1.0f);
    ui_data.xr_position = glm::vec2(0.5f, 0.5f);
    ui_data.aspect_ratio = fullscreen ? -1.0f : size.x / size.y;

    RenderStorage::get_singleton()->register_ui_widget(material->get_shader_ref(), quad_mesh, ui_data, 3, true);
}

sInputData XRPanel::get_input_data(bool ignore_focus)
{
    if (!XR_MANAGER->is_xr_available() || !(parameter_flags & CURVED_PANEL)) {
        return Panel2D::get_input_data(ignore_focus);
    }

    sInputData data;

    // Ray
    glm::vec3 ray_origin;
    glm::vec3 ray_direction;

    if (XR_MANAGER->is_xr_available()) {
        ray_origin = XR_MANAGER->get_controller_position(HAND_RIGHT, POSE_AIM);
        glm::mat4x4 select_hand_pose = XR_MANAGER->get_controller_pose(HAND_RIGHT, POSE_AIM);
        ray_direction = get_front(select_hand_pose);
    } else {
        Camera3D* camera = SIMULATION_MANAGER->get_main_camera();
        glm::vec3 ray_dir = camera->screen_to_ray(INPUT_MANAGER->get_mouse_position());
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
            &collision_dist,
            false);

    if (data.is_hovered) {
        glm::vec2 uv = glm::vec2(local_intersection_point) * 2.0f - 1.0f;

        float z_offset = 0.15f * (1.0f - (fabsf(uv.x * uv.x) * 0.5f + 0.5f));

        intersection_point += ray_direction * z_offset;
    }

    data.ray_intersection = intersection_point;
    data.ray_distance = glm::length(ray_origin - intersection_point);

    glm::vec2 local_pos = glm::vec2(local_intersection_point) / get_scale();
    data.local_position = glm::vec2(local_pos.x, size.y - local_pos.y);

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

        data.is_pressed &= IO::is_focused(this);
    }

    return data;
}

void XRPanel::update(float delta_time)
{
    if (!visibility) {
        return;
    }

    // reset event stuff..
    ui_data.flags &= ~(UI_DATA_HOVERED | UI_DATA_PRESSED | UI_DATA_SELECTED);

    // ignore focus process, doing it manually per button
    sInputData data = get_input_data(true);

    if (data.is_hovered) {
        if (is_button) {
            glm::vec2 local_pos = { data.local_position.x, -(data.local_position.y - size.y) };
            data.is_hovered &= local_pos.x > (button_position.x - button_size.x * 0.5f) && local_pos.x < (button_position.x + button_size.x * 0.5f);
            data.is_hovered &= local_pos.y > (button_position.y - button_size.y * 0.5f) && local_pos.y < (button_position.y + button_size.y * 0.5f);
        }

        // it's inside the button
        if (data.is_hovered) {
            if (is_button && data.was_pressed) {
                IO::set_focus(this);
            }

            data.is_pressed &= IO::is_focused(this);

            IO::push_input(this, data);
        }
    }

    if (!fullscreen) {
        ui_data.aspect_ratio = size.x / size.y;
    }

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

    if (data.was_released) {
        if (!on_pressed()) {
            Node::emit_signal(name, (void*)this);
        }
    }

    // Update uniforms
    ui_data.flags |= UI_DATA_HOVERED;
    ui_data.hover_time = 1.0f;

    if (data.is_pressed) {
        ui_data.flags |= UI_DATA_PRESSED;
    }

    on_hover = true;

    update_ui_data();

    return true;
}

void XRPanel::add_button(const std::string& signal, const std::string& texture_path, const glm::vec2& p, const glm::vec2& s, const Color& c)
{
    XRPanel* new_button = new XRPanel(signal, texture_path, { 0.0f, 0.0f }, size, Node2DFlags::CURVED_PANEL);
    add_child(new_button);

    new_button->set_priority(PANEL_BUTTON);

    new_button->ui_data.aspect_ratio = s.x / s.y;
    new_button->ui_data.xr_size = glm::clamp(s / size, 0.0f, 1.0f);
    new_button->ui_data.xr_position = glm::clamp(p / size, 0.0f, 1.0f);

    new_button->update_ui_data();

    new_button->make_as_button(s, p);
}
