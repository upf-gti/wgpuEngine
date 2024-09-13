#include "gizmo_3d.h"

#include <graphics/shader.h>
#include "graphics/renderer_storage.h"

#include <framework/nodes/mesh_instance_3d.h>
#include <framework/input.h>
#include <framework/math/intersections.h>
#include <framework/math/math_utils.h>
#include "framework/scene/parse_scene.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "shaders/AABB_shader.wgsl.gen.h"

#include "spdlog/spdlog.h"

Color Gizmo3D::X_AXIS_COLOR(0.89f, 0.01f, 0.02f, 1.0f);
Color Gizmo3D::Y_AXIS_COLOR(0.02f, 0.79f, 0.04f, 1.0f);
Color Gizmo3D::Z_AXIS_COLOR(0.03f, 0.04f, 0.8f, 1.0f);
Color Gizmo3D::AXIS_SELECTED_OFFSET_COLOR(0.5f, 0.5f, 0.5f, 0.0f);

void Gizmo3D::initialize(const eGizmoType& new_operation, const glm::vec3& position, const eGizmoAxis& new_axis)
{
    operation = new_operation;
    axis = axis;

    Material* material = new Material();
    material->set_depth_read(false);
    material->set_priority(0);
    material->set_transparency_type(ALPHA_BLEND);
    material->set_color(glm::vec4(1.0f));
    material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path));

    free_hand_point_mesh = parse_mesh("data/meshes/sphere.obj");
    free_hand_point_mesh->set_surface_material_override(free_hand_point_mesh->get_surface(0), material);

    if (operation & TRANSLATION_GIZMO) {
        init_translation_meshes();
    }

    if (operation & SCALE_GIZMO) {
        init_scale_meshes();
    }

    /*/if (operation & ROTATION_GIZMO) {
        init_rotation_meshes();
    }*/

    gizmo_position = position;
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
    delete arrow_mesh_x;
    delete arrow_mesh_y;
    delete arrow_mesh_z;

    delete scale_sphere_mesh_x;
    delete scale_sphere_mesh_y;
    delete scale_sphere_mesh_z;

    delete wire_circle_mesh_x;
    delete wire_circle_mesh_y;
    delete wire_circle_mesh_z;

    // TODO: delete materials
}

void Gizmo3D::set_operation(const eGizmoType& gizmo_type, const eGizmoAxis& axis)
{
    this->operation = gizmo_type;
    this->axis = axis;

    if (operation & TRANSLATION_GIZMO) {

        if (!arrow_mesh_x) {
            init_translation_meshes();
        }
    }

    if (operation & SCALE_GIZMO) {

        if (!scale_arrow_mesh_x) {
            init_scale_meshes();
        }
    }

    if (operation & ROTATION_GIZMO) {

        if (!wire_circle_mesh_x) {
            init_rotation_meshes();
        }
    }
}

bool Gizmo3D::update(Transform& t, const glm::vec3& controller_position, float delta_time)
{
    current_rotation = t.get_rotation();

    bool result = update(t.get_position_ref(), controller_position, delta_time);

    t.set_scale({ gizmo_scale.z, gizmo_scale.y, gizmo_scale .x});
    t.set_rotation(current_rotation);

    t.set_dirty(true);

    return result;
}

bool Gizmo3D::update(glm::vec3& new_position, glm::vec3& scale, const glm::vec3& controller_position, float delta_time)
{
    bool result = update(new_position, controller_position, delta_time);

    scale = { gizmo_scale.z, gizmo_scale.y, gizmo_scale.x };

    return result;
}

bool Gizmo3D::update(glm::vec3& new_position, glm::quat& rotation, const glm::vec3& controller_position, float delta_time)
{
    bool result = update(new_position, controller_position, delta_time);

    rotation = current_rotation;

    return result;
}

bool Gizmo3D::update(glm::vec3& new_position, const glm::vec3& controller_position, float delta_time)
{
    if (!enabled) {
        return false;
    }

    gizmo_position = new_position;

    if (!has_graved) {

        // The center of the gizmo servers as a free hand mode
        free_hand_selected = intersection::point_sphere(controller_position, gizmo_position, 0.01f);

        /*
            POSITION GIZMO TESTS
            Generate the AABB size of the bounding box of the arrow mesh
            Center the pointa ccordingly, and add a bit of margin (0.01f) for
            Grabing all axis in the bottom
        */

        if (!free_hand_selected) {

            if (operation & TRANSLATION_GIZMO) {

                if (axis & GIZMO_AXIS_X) {
                    glm::vec3 size = arrow_gizmo_scale * glm::vec3(mesh_size.y, mesh_size.x, mesh_size.z);
                    glm::vec3 box_center = gizmo_position + glm::vec3(size.x / 2.0f, 0.0f, 0.0f);
                    position_axis_selected.x = intersection::point_AABB(controller_position, box_center, size);
                }

                if (axis & GIZMO_AXIS_Y) {
                    glm::vec3 size = arrow_gizmo_scale * mesh_size;
                    glm::vec3 box_center = gizmo_position + glm::vec3(0.0f, size.y / 2.0f - 0.01f, 0.0f);
                    position_axis_selected.y = intersection::point_AABB(controller_position, box_center, size);
                }

                if (axis & GIZMO_AXIS_Z) {
                    glm::vec3 size = arrow_gizmo_scale * glm::vec3(mesh_size.x, mesh_size.z, mesh_size.y);
                    glm::vec3 box_center = gizmo_position + glm::vec3(0.0f, 0.0f, size.z / 2.0f - 0.01f);
                    position_axis_selected.z = intersection::point_AABB(controller_position, box_center, size);
                }
            }

            if (operation & SCALE_GIZMO) {

                if (axis & GIZMO_AXIS_X) {
                    glm::vec3 size = sphere_gizmo_scale * glm::vec3(mesh_size.x);
                    glm::vec3 box_center = gizmo_position + glm::vec3(sphere_gizmo_scale.x, 0.0f, 0.0f) * 2.5f;
                    scale_axis_selected.x = intersection::point_sphere(controller_position, box_center, size.x);
                }

                if (axis & GIZMO_AXIS_Y) {
                    glm::vec3 size = sphere_gizmo_scale * glm::vec3(mesh_size.x);
                    glm::vec3 box_center = gizmo_position + glm::vec3(0.0f, sphere_gizmo_scale.y, 0.0f) * 2.5f;
                    scale_axis_selected.y = intersection::point_sphere(controller_position, box_center, size.y);
                }

                if (axis & GIZMO_AXIS_Z) {
                    glm::vec3 size = sphere_gizmo_scale * glm::vec3(mesh_size.x);
                    glm::vec3 box_center = gizmo_position + glm::vec3(0.0f, 0.0f, sphere_gizmo_scale.z) * 2.5f;
                    scale_axis_selected.z = intersection::point_sphere(controller_position, box_center, size.z);
                }
            }
                
            /*if (operation & ROTATION_GIZMO) {

                float circle_scale = circle_gizmo_scale + 0.015f;
                float circle_radius = 0.02f;

                if (axis & GIZMO_AXIS_X) {
                    rotation_axis_selected.x = intersection::point_circle_ring(controller_position, gizmo_position, glm::vec3(1.0f, 0.0f, 0.0f), circle_scale, circle_radius);
                }

                if (axis & GIZMO_AXIS_Y) {
                    rotation_axis_selected.y = !rotation_axis_selected.x && intersection::point_circle_ring(controller_position, gizmo_position, glm::vec3(0.0f, 1.0f, 0.0f), circle_scale, circle_radius);
                }

                if (axis & GIZMO_AXIS_Z) {
                    rotation_axis_selected.z = !rotation_axis_selected.y && intersection::point_circle_ring(controller_position, gizmo_position, glm::vec3(0.0f, 0.0f, 1.0f), circle_scale, circle_radius);
                }
            }*/
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

        if (!has_graved) {
            reference_rotation_pose = controller_position;
            prev_controller_position = controller_position;
            rotation_diff = Input::get_controller_rotation(HAND_RIGHT) * glm::inverse(current_rotation);
            has_graved = true;
        }

        const glm::vec3& controller_delta = controller_position - prev_controller_position;

        if (free_hand_selected) {

            if (operation & TRANSLATION_GIZMO) {
                gizmo_position = controller_position;
            }

            if (operation & SCALE_GIZMO) {
                float dist = glm::length(reference_rotation_pose - controller_position);
                float prev_dist = glm::length(reference_rotation_pose - prev_controller_position);
                gizmo_scale += (dist - prev_dist) * 1e1f;
            }

            if (operation & ROTATION_GIZMO) {
                current_rotation = glm::inverse(current_rotation) * glm::inverse(rotation_diff) * Input::get_controller_rotation(HAND_RIGHT) * (current_rotation);
            }
        }
        else {

            if (operation & TRANSLATION_GIZMO) {
                const glm::vec3& constraint = { position_axis_selected.x ? 1.0f : 0.0f, position_axis_selected.y ? 1.0f : 0.0f, position_axis_selected.z ? 1.0f : 0.0f };
                gizmo_position += controller_delta * constraint;
            }

            if (operation & SCALE_GIZMO) {
                const glm::vec3& constraint = { scale_axis_selected.x ? 1.0f : 0.0f, scale_axis_selected.y ? 1.0f : 0.0f, scale_axis_selected.z ? 1.0f : 0.0f };
                gizmo_scale += controller_delta * constraint * 1e1f;
                gizmo_scale = glm::clamp(gizmo_scale, 0.0f, 4.0f);
            }

            //if (operation & ROTATION_GIZMO) {
            //    const glm::vec3& t = controller_position - reference_rotation_pose;

            //    // Normalize the points to the gizmo, for computeing the rotation delta
            //    glm::vec3 p1 = (reference_rotation_pose - gizmo_position);
            //    glm::vec3 p2 = (controller_position - gizmo_position);

            //    // Compute the rotation delta between the previous and the new position,
            //    // restricted for each axis
            //    glm::quat rot = { 0.0f, 0.0f, 0.0f, 1.0f };

            //    if (rotation_axis_selected.x) { p1.x = p2.x = 0.0f; }
            //    else if (rotation_axis_selected.y) { p1.y = p2.y = 0.0f; }
            //    else if (rotation_axis_selected.z) { p1.z = p2.z = 0.0f; }

            //    if (glm::any(rotation_axis_selected)) {
            //        rot = get_quat_between_vec3(p1, p2);
            //    }

            //    // Apply the rotation
            //    current_rotation = current_rotation * (glm::inverse(current_rotation) * rot * current_rotation);
            //    reference_rotation_pose = controller_position;
            //}
        }

        prev_controller_position = controller_position;
    }
    else {
        has_graved = false;
    }

    new_position = gizmo_position;

    return is_active;
}

void Gizmo3D::render(int axis)
{
    if (!enabled) {
        return;
    }

    free_hand_point_mesh->set_transform(Transform::identity());
    free_hand_point_mesh->translate(gizmo_position);
    free_hand_point_mesh->scale(glm::vec3(free_hand_selected ? 0.075f : 0.05f));
    free_hand_point_mesh->render();

    if (operation & TRANSLATION_GIZMO) {

        if (axis & GIZMO_AXIS_X)
        {
            arrow_mesh_x->set_transform(Transform::identity());
            arrow_mesh_x->translate(gizmo_position);
            arrow_mesh_x->rotate(glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));
            arrow_mesh_x->scale(arrow_gizmo_scale);
            arrow_mesh_x->get_surface_material_override(arrow_mesh_x->get_surface(0))->set_color(X_AXIS_COLOR + (position_axis_selected.x ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            arrow_mesh_x->render();
        }

        if (axis & GIZMO_AXIS_Y)
        {
            const glm::vec4 color = Y_AXIS_COLOR + (position_axis_selected.y ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f));
            arrow_mesh_y->set_transform(Transform::identity());
            arrow_mesh_y->translate(gizmo_position);
            arrow_mesh_y->scale(arrow_gizmo_scale);
            arrow_mesh_y->get_surface_material_override(arrow_mesh_y->get_surface(0))->set_color(color);
            arrow_mesh_y->render();
        }

        if (axis & GIZMO_AXIS_Z)
        {
            arrow_mesh_z->set_transform(Transform::identity());
            arrow_mesh_z->translate(gizmo_position);
            arrow_mesh_z->rotate(glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
            arrow_mesh_z->rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
            arrow_mesh_z->scale(arrow_gizmo_scale);
            arrow_mesh_z->get_surface_material_override(arrow_mesh_z->get_surface(0))->set_color(Z_AXIS_COLOR + (position_axis_selected.z ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            arrow_mesh_z->render();
        }
    }

    if (operation & SCALE_GIZMO) {

        if (axis & GIZMO_AXIS_X)
        {
            scale_sphere_mesh_x->set_transform(Transform::identity());
            scale_sphere_mesh_x->translate(gizmo_position + glm::vec3(sphere_gizmo_scale.x, 0.0f, 0.0f) * 2.5f);
            scale_sphere_mesh_x->scale(sphere_gizmo_scale);
            scale_sphere_mesh_x->get_surface_material_override(scale_sphere_mesh_x->get_surface(0))->set_color(X_AXIS_COLOR + (scale_axis_selected.x ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            scale_sphere_mesh_x->render();
        }

        if (axis & GIZMO_AXIS_Y)
        {
            scale_sphere_mesh_y->set_transform(Transform::identity());
            scale_sphere_mesh_y->translate(gizmo_position + glm::vec3(0.0f, sphere_gizmo_scale.y, 0.0f) * 2.5f);
            scale_sphere_mesh_y->scale(sphere_gizmo_scale);
            scale_sphere_mesh_y->get_surface_material_override(scale_sphere_mesh_y->get_surface(0))->set_color(Y_AXIS_COLOR + (scale_axis_selected.y ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            scale_sphere_mesh_y->render();
        }

        if (axis & GIZMO_AXIS_Z)
        {
            scale_sphere_mesh_z->set_transform(Transform::identity());
            scale_sphere_mesh_z->translate(gizmo_position + glm::vec3(0.0f, 0.0f, sphere_gizmo_scale.z) * 2.5f);
            scale_sphere_mesh_z->scale(sphere_gizmo_scale);
            scale_sphere_mesh_z->get_surface_material_override(scale_sphere_mesh_z->get_surface(0))->set_color(Z_AXIS_COLOR + (scale_axis_selected.z ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            scale_sphere_mesh_z->render();
        } 
    }

    /*if (operation & ROTATION_GIZMO) {

        if (axis & GIZMO_AXIS_X)
        {
            wire_circle_mesh_x->set_transform(Transform::identity());
            wire_circle_mesh_x->translate(gizmo_position);
            wire_circle_mesh_x->rotate(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
            wire_circle_mesh_x->scale(glm::vec3(circle_gizmo_scale));
            wire_circle_mesh_x->get_surface_material_override(0)->set_color(X_AXIS_COLOR + (rotation_axis_selected.x ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            wire_circle_mesh_x->render();
        }

        if (axis & GIZMO_AXIS_Y)
        {
            wire_circle_mesh_y->set_transform(Transform::identity());
            wire_circle_mesh_y->translate(gizmo_position);
            wire_circle_mesh_y->rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
            wire_circle_mesh_y->scale(glm::vec3(circle_gizmo_scale));
            wire_circle_mesh_y->get_surface_material_override(0)->set_color(Y_AXIS_COLOR + (rotation_axis_selected.y ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            wire_circle_mesh_y->render();
        }

        if (axis & GIZMO_AXIS_Z)
        {
            wire_circle_mesh_z->set_transform(Transform::identity());
            wire_circle_mesh_z->translate(gizmo_position);
            wire_circle_mesh_z->scale(glm::vec3(circle_gizmo_scale));
            wire_circle_mesh_z->get_surface_material_override(0)->set_color(Z_AXIS_COLOR + (rotation_axis_selected.z ? AXIS_SELECTED_OFFSET_COLOR : Color(0.f)));
            wire_circle_mesh_z->render();
        }
    }*/
}
