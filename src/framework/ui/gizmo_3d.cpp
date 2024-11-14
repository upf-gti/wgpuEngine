#include "gizmo_3d.h"

#include <graphics/shader.h>
#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"
#include "framework/camera/camera.h"

#include <framework/nodes/mesh_instance_3d.h>
#include <framework/input.h>
#include <framework/math/intersections.h>
#include <framework/math/math_utils.h>
#include "framework/parsers/parse_scene.h"
#include "framework/ui/io.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "shaders/AABB_shader.wgsl.gen.h"

#include "spdlog/spdlog.h"

Color Gizmo3D::X_AXIS_COLOR(0.89f, 0.01f, 0.02f, 1.0f);
Color Gizmo3D::Y_AXIS_COLOR(0.02f, 0.79f, 0.04f, 1.0f);
Color Gizmo3D::Z_AXIS_COLOR(0.03f, 0.04f, 0.8f, 1.0f);
Color Gizmo3D::AXIS_SELECTED_OFFSET_COLOR(0.4f, 0.4f, 0.4f, 0.0f);
Color Gizmo3D::AXIS_NOT_SELECTED_COLOR(0.2f, 0.2f, 0.2f, 1.0f);

void Gizmo3D::initialize(const eGizmoOp& new_operation, const glm::vec3& position)
{
    operation = new_operation;

    xr_enabled = Renderer::instance->get_openxr_available();

    if (xr_enabled) {
        Material* material = new Material();
        material->set_depth_read(false);
        material->set_priority(0);
        material->set_transparency_type(ALPHA_BLEND);
        material->set_color(glm::vec4(1.0f));
        material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path));

        free_hand_point_mesh = parse_mesh("data/meshes/sphere.obj");
        free_hand_point_mesh->set_surface_material_override(free_hand_point_mesh->get_surface(0), material);

        if (operation & TRANSLATE) {
            init_translation_meshes();
        }

        if (operation & SCALE) {
            init_scale_meshes();
        }

        if (operation & ROTATE) {
            init_rotation_meshes();
        }
    }
    else {
        gizmo_2d.set_operation(new_operation);
    }

    transform.set_position(position);
}

void Gizmo3D::init_translation_meshes()
{
    arrow_mesh_x = parse_mesh("data/meshes/arrow.obj");

    Material* material_x = new Material();
    material_x->set_depth_read(false);
    material_x->set_priority(0);
    material_x->set_type(MATERIAL_UNLIT);
    material_x->set_transparency_type(ALPHA_BLEND);
    material_x->set_color(colors::RED);
    material_x->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_x));
    arrow_mesh_x->set_surface_material_override(arrow_mesh_x->get_surface(0), material_x);
    arrow_mesh_x->set_surface_material_override(arrow_mesh_x->get_surface(1), material_x);

    arrow_mesh_y = parse_mesh("data/meshes/arrow.obj");
    Material* material_y = new Material();
    material_y->set_depth_read(false);
    material_y->set_priority(0);
    material_y->set_type(MATERIAL_UNLIT);
    material_y->set_transparency_type(ALPHA_BLEND);
    material_y->set_color(colors::GREEN);
    material_y->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_y));
    arrow_mesh_y->set_surface_material_override(arrow_mesh_y->get_surface(0), material_y);
    arrow_mesh_y->set_surface_material_override(arrow_mesh_y->get_surface(1), material_y);

    arrow_mesh_z = parse_mesh("data/meshes/arrow.obj");
    Material* material_z = new Material();
    material_z->set_depth_read(false);
    material_z->set_priority(0);
    material_z->set_type(MATERIAL_UNLIT);
    material_z->set_transparency_type(ALPHA_BLEND);
    material_z->set_color(colors::BLUE);
    material_z->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_z));
    arrow_mesh_z->set_surface_material_override(arrow_mesh_z->get_surface(0), material_z);
    arrow_mesh_z->set_surface_material_override(arrow_mesh_z->get_surface(1), material_z);
}

void Gizmo3D::init_scale_meshes()
{
    // Sphere for the scale gizmo with rotation and position
    {
        scale_sphere_mesh_x = parse_mesh("data/meshes/sphere.obj");
        Material* material_x_sphere = new Material();
        material_x_sphere->set_depth_read(false);
        material_x_sphere->set_priority(0);
        material_x_sphere->set_transparency_type(ALPHA_BLEND);
        material_x_sphere->set_color(colors::RED);
        material_x_sphere->set_type(MATERIAL_UNLIT);
        material_x_sphere->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_x_sphere));
        scale_sphere_mesh_x->set_surface_material_override(scale_sphere_mesh_x->get_surface(0), material_x_sphere);

        scale_sphere_mesh_y = parse_mesh("data/meshes/sphere.obj");
        Material* material_y = new Material();
        material_y->set_depth_read(false);
        material_y->set_priority(0);
        material_y->set_transparency_type(ALPHA_BLEND);
        material_y->set_color(colors::GREEN);
        material_y->set_type(MATERIAL_UNLIT);
        material_y->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_y));
        scale_sphere_mesh_y->set_surface_material_override(scale_sphere_mesh_y->get_surface(0), material_y);

        scale_sphere_mesh_z = parse_mesh("data/meshes/sphere.obj");
        Material* material_z = new Material();
        material_z->set_depth_read(false);
        material_z->set_priority(0);
        material_z->set_transparency_type(ALPHA_BLEND);
        material_z->set_color(colors::BLUE);
        material_z->set_type(MATERIAL_UNLIT);
        material_z->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_z));
        scale_sphere_mesh_z->set_surface_material_override(scale_sphere_mesh_z->get_surface(0), material_z);
    }
}

void Gizmo3D::init_rotation_meshes()
{
    wire_circle_mesh_x = parse_mesh("data/meshes/wired_circle.obj");
    Material* material_x = new Material();
    material_x->set_depth_read(false);
    material_x->set_priority(0);
    material_x->set_transparency_type(ALPHA_BLEND);
    material_x->set_color(colors::RED);
    material_x->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_x));
    wire_circle_mesh_x->set_surface_material_override(wire_circle_mesh_x->get_surface(0), material_x);

    wire_circle_mesh_y = parse_mesh("data/meshes/wired_circle.obj");
    Material* material_y = new Material();
    material_y->set_depth_read(false);
    material_y->set_priority(0);
    material_y->set_transparency_type(ALPHA_BLEND);
    material_y->set_color(colors::GREEN);
    material_y->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_y));
    wire_circle_mesh_y->set_surface_material_override(wire_circle_mesh_y->get_surface(0), material_y);

    wire_circle_mesh_z = parse_mesh("data/meshes/wired_circle.obj");
    Material*material_z = new Material();
    material_z->set_depth_read(false);
    material_z->set_priority(0);
    material_z->set_transparency_type(ALPHA_BLEND);
    material_z->set_color(colors::BLUE);
    material_z->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material_z));
    wire_circle_mesh_z->set_surface_material_override(wire_circle_mesh_z->get_surface(0), material_z);
}

void Gizmo3D::clean()
{
    if (arrow_mesh_x) delete arrow_mesh_x;
    if (arrow_mesh_y) delete arrow_mesh_y;
    if (arrow_mesh_z) delete arrow_mesh_z;

    if (scale_sphere_mesh_x) delete scale_sphere_mesh_x;
    if (scale_sphere_mesh_y) delete scale_sphere_mesh_y;
    if (scale_sphere_mesh_z) delete scale_sphere_mesh_z;

    if (wire_circle_mesh_x) delete wire_circle_mesh_x;
    if (wire_circle_mesh_y) delete wire_circle_mesh_y;
    if (wire_circle_mesh_z) delete wire_circle_mesh_z;
}

void Gizmo3D::set_transform(const Transform& t)
{
    transform = t;
}

void Gizmo3D::set_enabled(bool new_enabled)
{
    enabled = new_enabled;
}

void Gizmo3D::set_operation(const eGizmoOp& gizmo_type)
{
    // Force enabled true when selection an option?
    set_enabled(true);

    if (xr_enabled) {
        operation = gizmo_type;

        if (operation & TRANSLATE) {

            if (!arrow_mesh_x) {
                init_translation_meshes();
            }
        }

        if (operation & SCALE) {

            if (!scale_arrow_mesh_x) {
                init_scale_meshes();
            }
        }

        if (operation & ROTATE) {

            if (!wire_circle_mesh_x) {
                init_rotation_meshes();
            }
        }
    }
    else {
        gizmo_2d.set_operation(gizmo_type);
    }
}

bool Gizmo3D::update(const glm::vec3& controller_position, float delta_time)
{
    return update(transform.get_position_ref(), controller_position, delta_time);
}

bool Gizmo3D::update(Transform& t, const glm::vec3& controller_position, float delta_time)
{
    transform.set_rotation(t.get_rotation());
    transform.set_scale(t.get_scale());

    bool result = update(t.get_position_ref(), controller_position, delta_time);

    const glm::vec3& gizmo_scale = transform.get_scale();
    t.set_scale({ gizmo_scale.z, gizmo_scale.y, gizmo_scale.x });
    t.set_rotation(transform.get_rotation());

    t.set_dirty(true);

    return result;
}

bool Gizmo3D::update(glm::vec3& new_position, glm::vec3& scale, const glm::vec3& controller_position, float delta_time)
{
    bool result = update(new_position, controller_position, delta_time);

    const glm::vec3& gizmo_scale = transform.get_scale();
    scale = { gizmo_scale.z, gizmo_scale.y, gizmo_scale.x };

    return result;
}

bool Gizmo3D::update(glm::vec3& new_position, glm::quat& rotation, const glm::vec3& controller_position, float delta_time)
{
    bool result = update(new_position, controller_position, delta_time);

    rotation = transform.get_rotation();

    return result;
}

bool Gizmo3D::update(glm::vec3& new_position, const glm::vec3& controller_position, float delta_time)
{
    if (!enabled) {
        return false;
    }

    transform.set_position(new_position);

    if (!has_graved) {

        glm::vec3 current_position = transform.get_position();

        // The center of the gizmo servers as a free hand mode
        free_hand_selected = intersection::point_sphere(controller_position, current_position, 0.01f);

        /*
            POSITION GIZMO TESTS
            Generate the AABB size of the bounding box of the arrow mesh
            Center the pointa ccordingly, and add a bit of margin (0.01f) for
            Grabing all axis in the bottom
        */

        if (!free_hand_selected) {

            if (operation & TRANSLATE) {

                if (operation & TRANSLATE_X) {
                    glm::vec3 size = arrow_gizmo_scale * glm::vec3(mesh_size.y, mesh_size.x, mesh_size.z);
                    glm::vec3 box_center = current_position + glm::vec3(size.x / 2.0f, 0.0f, 0.0f);
                    position_axis_selected.x = intersection::point_AABB(controller_position, box_center, size);
                }

                if (operation & TRANSLATE_Y) {
                    glm::vec3 size = arrow_gizmo_scale * mesh_size;
                    glm::vec3 box_center = current_position + glm::vec3(0.0f, size.y / 2.0f - 0.01f, 0.0f);
                    position_axis_selected.y = intersection::point_AABB(controller_position, box_center, size);
                }

                if (operation & TRANSLATE_Z) {
                    glm::vec3 size = arrow_gizmo_scale * glm::vec3(mesh_size.x, mesh_size.z, mesh_size.y);
                    glm::vec3 box_center = current_position + glm::vec3(0.0f, 0.0f, size.z / 2.0f - 0.01f);
                    position_axis_selected.z = intersection::point_AABB(controller_position, box_center, size);
                }
            }

            if (operation & SCALE) {

                if (operation & SCALE_X) {
                    glm::vec3 size = sphere_gizmo_scale * glm::vec3(mesh_size.x);
                    glm::vec3 box_center = current_position + glm::vec3(sphere_gizmo_scale.x, 0.0f, 0.0f) * 2.5f;
                    scale_axis_selected.x = intersection::point_sphere(controller_position, box_center, size.x);
                }

                if (operation & SCALE_Y) {
                    glm::vec3 size = sphere_gizmo_scale * glm::vec3(mesh_size.x);
                    glm::vec3 box_center = current_position + glm::vec3(0.0f, sphere_gizmo_scale.y, 0.0f) * 2.5f;
                    scale_axis_selected.y = intersection::point_sphere(controller_position, box_center, size.y);
                }

                if (operation & SCALE_Z) {
                    glm::vec3 size = sphere_gizmo_scale * glm::vec3(mesh_size.x);
                    glm::vec3 box_center = current_position + glm::vec3(0.0f, 0.0f, sphere_gizmo_scale.z) * 2.5f;
                    scale_axis_selected.z = intersection::point_sphere(controller_position, box_center, size.z);
                }
            }

            if (operation & ROTATE) {

                float circle_scale = circle_gizmo_scale + 0.025f;
                float circle_radius = 0.05f;

                if (operation & ROTATE_X) {
                    rotation_axis_selected.x = intersection::point_circle_ring(controller_position, current_position, glm::vec3(1.0f, 0.0f, 0.0f), circle_scale, circle_radius);
                }

                if (operation & ROTATE_Y) {
                    rotation_axis_selected.y = !rotation_axis_selected.x && intersection::point_circle_ring(controller_position, current_position, glm::vec3(0.0f, 1.0f, 0.0f), circle_scale, circle_radius);
                }

                if (operation & ROTATE_Z) {
                    rotation_axis_selected.z = !rotation_axis_selected.y && intersection::point_circle_ring(controller_position, current_position, glm::vec3(0.0f, 0.0f, 1.0f), circle_scale, circle_radius);
                }
            }
        }
        else {
            position_axis_selected = glm::bvec3(false);
            scale_axis_selected = glm::bvec3(false);
            rotation_axis_selected = glm::bvec3(false);
        }
    }

    const bool is_active = free_hand_selected || glm::any(position_axis_selected) || glm::any(scale_axis_selected) || glm::any(rotation_axis_selected);

    // Calculate the movement vector for the gizmo
    if (Input::get_trigger_value(HAND_RIGHT) > 0.5f) {

        glm::quat current_hand_rotation = Input::get_controller_rotation(HAND_RIGHT);
        glm::vec3 current_hand_translation = Input::get_controller_position(HAND_RIGHT);

        if (!has_graved) {
            start_hand_translation = current_hand_translation;
            last_hand_translation = current_hand_translation;
            last_hand_rotation = current_hand_rotation;
            has_graved = true;
        }

        const glm::quat& rotation_diff = (current_hand_rotation * glm::inverse(last_hand_rotation));
        const glm::vec3& translation_diff = current_hand_translation - last_hand_translation;

        if (free_hand_selected) {

            bool free_hand_scale = !(operation & TRANSLATE) && !(operation & ROTATE);

            if (operation & TRANSLATE) {
                transform.set_position(controller_position);
            }

            if (operation & ROTATE) {
                glm::quat current_rotation = transform.get_rotation();
                transform.set_rotation(current_rotation * (glm::inverse(current_rotation) * rotation_diff * current_rotation));
            }

            if (free_hand_scale && (operation & SCALE)) {
                glm::vec3 current_position = transform.get_position();
                float dist = glm::length(current_hand_translation - current_position);
                float prev_dist = glm::length(last_hand_translation - current_position);
                transform.set_scale(transform.get_scale() + (dist - prev_dist) * 1e1f);
            }
        }
        else {

            if (operation & TRANSLATE) {
                const glm::vec3& constraint = { position_axis_selected.x ? 1.0f : 0.0f, position_axis_selected.y ? 1.0f : 0.0f, position_axis_selected.z ? 1.0f : 0.0f };
                transform.translate(translation_diff* constraint);
            }

            if (operation & SCALE) {
                // TODO: local scale, redo it to scale in world
                const glm::vec3& constraint = { scale_axis_selected.x ? 1.0f : 0.0f, scale_axis_selected.y ? 1.0f : 0.0f, scale_axis_selected.z ? 1.0f : 0.0f };
                glm::vec3 current_scale = transform.get_scale();
                current_scale += translation_diff * constraint * 1e1f;
                transform.set_scale(glm::clamp(current_scale, 0.0f, 4.0f));
            }

            if (operation & ROTATE) {

                // Normalize the points to the gizmo, for computeing the rotation delta
                glm::vec3 current_position = transform.get_position();
                glm::vec3 p1 = (current_hand_translation - current_position);
                glm::vec3 p2 = (last_hand_translation - current_position);

                // Compute the rotation delta between the previous and the new position,
                // restricted for each axis
                glm::quat rot = { 0.0f, 0.0f, 0.0f, 1.0f };

                if (rotation_axis_selected.x) { p1.x = p2.x = 0.0f; }
                else if (rotation_axis_selected.y) { p1.y = p2.y = 0.0f; }
                else if (rotation_axis_selected.z) { p1.z = p2.z = 0.0f; }

                if (glm::any(rotation_axis_selected)) {
                    rot = get_quat_between_vec3(p1, p2);
                }

                // Apply the rotation
                glm::quat current_rotation = transform.get_rotation();
                transform.set_rotation(current_rotation * (glm::inverse(current_rotation) * glm::inverse(rot) * current_rotation));
            }
        }

        last_hand_rotation = current_hand_rotation;
        last_hand_translation = current_hand_translation;
    }
    else {
        has_graved = false;
    }

    IO::set_want_capture_input(is_active && has_graved);

    new_position = transform.get_position();

    return has_graved;
}

bool Gizmo3D::render()
{
    if (!enabled) {
        return false;
    }

    if (xr_enabled) {
        render_xr();
    }
    else {
        Camera* camera = Renderer::instance->get_camera();
        glm::mat4x4 m = Transform::transform_to_mat4(transform);
        if (gizmo_2d.render(camera->get_view(), camera->get_projection(), m)) {
            transform = Transform::mat4_to_transform(m);
            return true;
        }
    }

    return false;
}

void Gizmo3D::render_xr()
{
    const glm::vec3& current_position = transform.get_position();

    free_hand_point_mesh->set_transform(Transform::identity());
    free_hand_point_mesh->translate(current_position);
    free_hand_point_mesh->scale(glm::vec3(free_hand_selected ? 0.075f : 0.05f));
    free_hand_point_mesh->render();

    if (operation & TRANSLATE) {

        if (operation & TRANSLATE_X)
        {
            arrow_mesh_x->set_transform(Transform::identity());
            arrow_mesh_x->translate(current_position);
            arrow_mesh_x->rotate(glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));
            arrow_mesh_x->scale(arrow_gizmo_scale);
            const Color& color = position_axis_selected.x ? X_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (position_axis_selected.y || position_axis_selected.z) ? AXIS_NOT_SELECTED_COLOR : X_AXIS_COLOR;
            arrow_mesh_x->get_surface_material_override(arrow_mesh_x->get_surface(0))->set_color(color);
            arrow_mesh_x->render();
        }

        if (operation & TRANSLATE_Y)
        {
            arrow_mesh_y->set_transform(Transform::identity());
            arrow_mesh_y->translate(current_position);
            arrow_mesh_y->scale(arrow_gizmo_scale);
            const Color& color = position_axis_selected.y ? Y_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (position_axis_selected.x || position_axis_selected.z) ? AXIS_NOT_SELECTED_COLOR : Y_AXIS_COLOR;
            arrow_mesh_y->get_surface_material_override(arrow_mesh_y->get_surface(0))->set_color(color);
            arrow_mesh_y->render();
        }

        if (operation & TRANSLATE_Z)
        {
            arrow_mesh_z->set_transform(Transform::identity());
            arrow_mesh_z->translate(current_position);
            arrow_mesh_z->rotate(glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
            arrow_mesh_z->rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
            arrow_mesh_z->scale(arrow_gizmo_scale);
            const Color& color = position_axis_selected.z ? Z_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (position_axis_selected.x || position_axis_selected.y) ? AXIS_NOT_SELECTED_COLOR : Z_AXIS_COLOR;
            arrow_mesh_z->get_surface_material_override(arrow_mesh_z->get_surface(0))->set_color(color);
            arrow_mesh_z->render();
        }
    }

    if (operation & SCALE) {

        if (operation & SCALE_X)
        {
            scale_sphere_mesh_x->set_transform(Transform::identity());
            scale_sphere_mesh_x->translate(current_position + glm::vec3(sphere_gizmo_scale.x, 0.0f, 0.0f) * 2.5f);
            scale_sphere_mesh_x->scale(sphere_gizmo_scale);
            const Color& color = scale_axis_selected.x ? X_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (scale_axis_selected.y || scale_axis_selected.z) ? AXIS_NOT_SELECTED_COLOR : X_AXIS_COLOR;
            scale_sphere_mesh_x->get_surface_material_override(scale_sphere_mesh_x->get_surface(0))->set_color(color);
            scale_sphere_mesh_x->render();
        }

        if (operation & SCALE_Y)
        {
            scale_sphere_mesh_y->set_transform(Transform::identity());
            scale_sphere_mesh_y->translate(current_position + glm::vec3(0.0f, sphere_gizmo_scale.y, 0.0f) * 2.5f);
            scale_sphere_mesh_y->scale(sphere_gizmo_scale);
            const Color& color = scale_axis_selected.y ? Y_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (scale_axis_selected.x || scale_axis_selected.z) ? AXIS_NOT_SELECTED_COLOR : Y_AXIS_COLOR;
            scale_sphere_mesh_y->get_surface_material_override(scale_sphere_mesh_y->get_surface(0))->set_color(color);
            scale_sphere_mesh_y->render();
        }

        if (operation & SCALE_Z)
        {
            scale_sphere_mesh_z->set_transform(Transform::identity());
            scale_sphere_mesh_z->translate(current_position + glm::vec3(0.0f, 0.0f, sphere_gizmo_scale.z) * 2.5f);
            scale_sphere_mesh_z->scale(sphere_gizmo_scale);
            const Color& color = scale_axis_selected.z ? Z_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (scale_axis_selected.x || scale_axis_selected.y) ? AXIS_NOT_SELECTED_COLOR : Z_AXIS_COLOR;
            scale_sphere_mesh_z->get_surface_material_override(scale_sphere_mesh_z->get_surface(0))->set_color(color);
            scale_sphere_mesh_z->render();
        }
    }

    if (operation & ROTATE) {

        if (operation & ROTATE_X)
        {
            wire_circle_mesh_x->set_transform(Transform::identity());
            wire_circle_mesh_x->translate(current_position);
            wire_circle_mesh_x->rotate(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
            wire_circle_mesh_x->scale(glm::vec3(circle_gizmo_scale));
            const Color& color = rotation_axis_selected.x ? X_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (rotation_axis_selected.y || rotation_axis_selected.z) ? AXIS_NOT_SELECTED_COLOR : X_AXIS_COLOR;
            wire_circle_mesh_x->get_surface_material_override(wire_circle_mesh_x->get_surface(0))->set_color(color);
            wire_circle_mesh_x->render();
        }

        if (operation & ROTATE_Y)
        {
            wire_circle_mesh_y->set_transform(Transform::identity());
            wire_circle_mesh_y->translate(current_position);
            wire_circle_mesh_y->rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
            wire_circle_mesh_y->scale(glm::vec3(circle_gizmo_scale));
            const Color& color = rotation_axis_selected.y ? Y_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (rotation_axis_selected.x || rotation_axis_selected.z) ? AXIS_NOT_SELECTED_COLOR : Y_AXIS_COLOR;
            wire_circle_mesh_y->get_surface_material_override(wire_circle_mesh_y->get_surface(0))->set_color(color);
            wire_circle_mesh_y->render();
        }

        if (operation & ROTATE_Z)
        {
            wire_circle_mesh_z->set_transform(Transform::identity());
            wire_circle_mesh_z->translate(current_position);
            wire_circle_mesh_z->scale(glm::vec3(circle_gizmo_scale));
            const Color& color = rotation_axis_selected.z ? Z_AXIS_COLOR + AXIS_SELECTED_OFFSET_COLOR : (rotation_axis_selected.x || rotation_axis_selected.y) ? AXIS_NOT_SELECTED_COLOR : Z_AXIS_COLOR;
            wire_circle_mesh_z->get_surface_material_override(wire_circle_mesh_z->get_surface(0))->set_color(color);
            wire_circle_mesh_z->render();
        }
    }
}
