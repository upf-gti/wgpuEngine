#include "panel_2d.h"

#include "core/managers/engine/engine_manager.h"
#include "core/managers/input/input_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/simulation/simulation_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "framework/math/intersections.h"
#include "framework/ui/io.h"
#include "framework/utils/utils.h"
#include "scene/3d/camera_3d.h"

#include "scene/2d/button_2d.h"
#include "scene/3d/text_3d.h"

#include "shaders/mesh_forward.wgsl.gen.h"
#include "shaders/ui/ui_panel.wgsl.gen.h"
#include "shaders/ui/ui_texture.wgsl.gen.h"

#include <cctype>

Panel2D::Panel2D(const std::string& name, const glm::vec2& pos, const glm::vec2& size, uint32_t flags, const Color& col) :
        Panel2D(name, "", pos, size, flags, col) {}

Panel2D::Panel2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint32_t flags, const Color& c) :
        Node2D(name, p, s, flags), color(c)
{
    class_type = Node2DClassType::PANEL;

    Material* material = new Material();
    material->set_color(color);
    material->set_type(MATERIAL_UI);
    material->set_is_2D(true);
    material->set_cull_type(CULL_BACK);
    material->set_transparency_type(ALPHA_BLEND);
    material->set_depth_read_write(false);
    material->set_priority(class_type);

    if (image_path.size()) {
        material->set_diffuse_texture(RenderStorage::get_singleton()->get_texture(image_path, TEXTURE_STORAGE_UI));
    }

    material->set_shader(RenderStorage::get_singleton()->get_singleton()->get_shader_from_source(shaders::ui_panel::source, shaders::ui_panel::path, shaders::ui_panel::libraries, material));

    Surface* quad_surface = new Surface();
    quad_surface->create_quad(size.x, size.y, true);

    quad_mesh = new Mesh();
    quad_mesh->add_surface(quad_surface);
    quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

    if (XR_MANAGER->is_xr_available()) {
        disable_2d();
    }

    RenderStorage::get_singleton()->register_ui_widget(material->get_shader_ref(), quad_mesh, ui_data, 3);

    parameter_flags = flags;
}

Panel2D::~Panel2D()
{
    RenderStorage::get_singleton()->delete_ui_widget(quad_mesh);

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
    if (!visibility) {
        return;
    }

    if (render_background) {
        // Convert the mat3x3 to mat4x4
        uint8_t priority = class_type;
        glm::vec2 position = get_translation() + size * 0.5f * get_scale();
        // Don't apply the scaling to combo buttons..
        glm::vec2 scale = get_scale() * (class_type != Node2DClassType::COMBO_BUTTON ? scaling : glm::vec2(scaling.x, 1.0f));
        glm::mat4x4 model = glm::translate(glm::mat4x4(1.0f), glm::vec3(position, -priority * 3e-5));
        model = glm::scale(model, glm::vec3(scale, 1.0f));
        model = get_global_viewport_model() * model;
        //RENDER_MANAGER->add_renderable(quad_mesh, model);
    }

    Node2D::render();
}

bool Panel2D::was_input_pressed()
{
    Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

    bool is_2d = material->get_is_2D() || (!XR_MANAGER->is_xr_available());

    if (is_2d) {
        return INPUT_MANAGER->was_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
    } else {
        return XR_MANAGER->was_button_pressed(XR_BUTTON_A) || XR_MANAGER->was_trigger_pressed(HAND_RIGHT);
    }

    return false;
}

bool Panel2D::was_input_released()
{
    Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

    bool is_2d = material->get_is_2D() || (!XR_MANAGER->is_xr_available());

    if (is_2d) {
        return INPUT_MANAGER->was_mouse_released(GLFW_MOUSE_BUTTON_LEFT);
    } else {
        return XR_MANAGER->was_button_released(XR_BUTTON_A) || XR_MANAGER->was_trigger_released(HAND_RIGHT);
    }

    return false;
}

bool Panel2D::is_input_pressed()
{
    Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

    bool is_2d = material->get_is_2D() || (!XR_MANAGER->is_xr_available());

    if (is_2d) {
        return INPUT_MANAGER->is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
    } else {
        return XR_MANAGER->is_button_pressed(XR_BUTTON_A) || XR_MANAGER->is_trigger_pressed(HAND_RIGHT);
    }

    return false;
}

sInputData Panel2D::get_input_data(bool ignore_focus)
{
    sInputData data;

    Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

    if (material->get_is_2D()) {
        const glm::vec2& mouse_pos = INPUT_MANAGER->get_mouse_position();
        const glm::vec2& min = get_translation();
        const glm::vec2& max = min + size;

        data.is_hovered = mouse_pos.x >= min.x && mouse_pos.y >= min.y && mouse_pos.x <= max.x && mouse_pos.y <= max.y;

        const glm::vec2& local_mouse_pos = mouse_pos - get_translation();
        data.local_position = glm::vec2(local_mouse_pos.x, size.y - local_mouse_pos.y);
    } else {
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
            if (XR_MANAGER->is_xr_available()) {
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

        data.is_pressed &= IO::is_focused(this);
    }

    return data;
}

bool Panel2D::on_input(sInputData data)
{
    if (data.was_pressed) {
        last_press_time = static_cast<float>(glfwGetTime());
    }

    if (data.is_pressed && (parameter_flags & LONG_CLICK) && (glfwGetTime() - last_press_time) > 0.5f) {
        Node::emit_signal(name + "@long_click", (void*)nullptr);
    }

    if (data.was_hovered) {
        XR_MANAGER->vibrate_hand(HAND_RIGHT, HOVER_HAPTIC_AMPLITUDE, HOVER_HAPTIC_DURATION);
    }

    return true;
}

bool Panel2D::on_pressed()
{
    float now = static_cast<float>(glfwGetTime());

    bool skip = false;

    if ((parameter_flags & DBL_CLICK) && (now - last_release_time) < 0.4f) {
        Node::emit_signal(name + "@dbl_click", (void*)nullptr);
        skip = true;
    }

    // Close submenu when single press for buttons
    else if (class_type == BUTTON || class_type == TEXTURE_BUTTON) {
        auto parent_2d = get_parent<Node2D*>();

        if (parent_2d) {
            // Get parent of the group..
            if (parent_2d->get_class_type() == GROUP) {
                parent_2d = parent_2d->get_parent<Node2D*>();
            }

            // By now we should have the submenu box, so go up again
            parent_2d = parent_2d->get_parent<Node2D*>();

            ButtonSubmenu2D* submenu = dynamic_cast<ButtonSubmenu2D*>(parent_2d);
            if (submenu && !(submenu->get_flags() & KEEP_SUBMENU_OPENED)) {
                submenu->box->set_visibility(false);
            }
        }
    }

    last_release_time = now;

    return skip;
}

void Panel2D::update_scroll_view()
{
    auto parent_2d = get_parent<Node2D*>();

    if (!(parameter_flags & SCROLLABLE) || !parent_2d) {
        return;
    }

    // Last chance..
    parent_2d = parent_2d->get_parent<Node2D*>();

    if (!parent_2d || parent_2d->get_class_type() != VCONTAINER) {
        return;
    }

    ui_data.clip_range = 1.0f;

    set_visibility(true, false);

    float size_y = (get_size() * get_scale()).y;
    float parent_size_y = (parent_2d->get_size() * parent_2d->get_scale()).y;
    float y = get_translation().y;
    float parent_y = parent_2d->get_translation().y;
    float y_min = parent_y;
    float y_max = parent_y + parent_size_y - size_y;
    // exceeds at the top
    if (y < y_min) {
        ui_data.clip_range = glm::clamp((y - y_min) / size_y, -1.0f, -0.01f);

        if (class_type == TEXT_SHADOW) {
            set_visibility(false, false);
        }
    }
    // exceeds at the bottom
    else if (y > y_max) {
        ui_data.clip_range = 1.0f - glm::clamp((y - y_max) / size_y, 0.0f, 0.99f);

        if (class_type == TEXT_SHADOW) {
            set_visibility(false, false);
        }
    }

    if (glm::abs(ui_data.clip_range) < 0.2f) {
        set_visibility(false, false);
    }
}

void Panel2D::disable_2d()
{
    Material* material = quad_mesh->get_surface_material_override(quad_mesh->get_surface(0));

    material->set_is_2D(false);
    material->set_depth_read(true);

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
    RenderStorage::get_singleton()->update_ui_widget(quad_mesh, ui_data);
}

/*
 *   XRPanel
 */

/*
 *   Text
 */

/*
 *	ColorPicker
 */
