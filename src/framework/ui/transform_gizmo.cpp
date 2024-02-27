#include "transform_gizmo.h"

#include <graphics/shader.h>
#include <framework/entities/entity_mesh.h>
#include <framework/input.h>
#include <framework/utils/intersections.h>
#include <framework/colors.h>
#include <framework/utils/utils.h>

#include "framework/scene/parse_scene.h"
#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"

void TransformGizmo::initialize(const eGizmoType& gizmo_type, const glm::vec3 &position, const eGizmoAxis& axis)
{
	this->type = gizmo_type;
    this->axis = axis;

    Material m;
    m.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl");
    m.flags = MATERIAL_COLOR;

    if (gizmo_type & POSITION_GIZMO) {
        init_arrow_meshes();
    }

    if (gizmo_type & ROTATION_GIZMO) {
        init_circle_meshes();
    }

	position_gizmo_scale = glm::vec3(0.1f, 0.1f, 0.1f);
    rotation_gizmo_scale = glm::vec3(0.01f, 0.01f, 0.01f);
    gizmo_position = position;
}

void TransformGizmo::init_arrow_meshes()
{
    Material m;
    m.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl");
    m.flags = MATERIAL_COLOR;

    arrow_mesh_x = parse_mesh("data/meshes/arrow.obj");
    arrow_mesh_x->set_surface_material_override(arrow_mesh_x->get_surface(0), m);

    arrow_mesh_y = parse_mesh("data/meshes/arrow.obj");
    arrow_mesh_y->set_surface_material_override(arrow_mesh_y->get_surface(0), m);

    arrow_mesh_z = parse_mesh("data/meshes/arrow.obj");
    arrow_mesh_z->set_surface_material_override(arrow_mesh_z->get_surface(0), m);
}

void TransformGizmo::init_circle_meshes()
{
    Material m;
    m.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl");
    m.flags = MATERIAL_COLOR;

    wire_circle_mesh_x = parse_mesh("data/meshes/wired_circle.obj");
    wire_circle_mesh_x->set_surface_material_override(wire_circle_mesh_x->get_surface(0), m);

    wire_circle_mesh_y = parse_mesh("data/meshes/wired_circle.obj");
    wire_circle_mesh_y->set_surface_material_override(wire_circle_mesh_y->get_surface(0), m);

    wire_circle_mesh_z = parse_mesh("data/meshes/wired_circle.obj");
    wire_circle_mesh_z->set_surface_material_override(wire_circle_mesh_z->get_surface(0), m);
}

void TransformGizmo::clean()
{
	delete arrow_mesh_x;
    delete arrow_mesh_y;
    delete arrow_mesh_z;

    delete wire_circle_mesh_x;
    delete wire_circle_mesh_y;
    delete wire_circle_mesh_z;
}

void TransformGizmo::set_mode(const eGizmoType& gizmo_type, const eGizmoAxis& axis)
{
    this->type = gizmo_type;
    this->axis = axis;

    if (type & POSITION_GIZMO) {

        if (!arrow_mesh_x) {
            init_arrow_meshes();
        }
    }

    if (type & ROTATION_GIZMO) {

        if (!wire_circle_mesh_x) {
            init_circle_meshes();
        }
    }
}

float get_angle(const glm::vec2& v1, const glm::vec2& v2)
{
    const float dot = glm::dot(glm::normalize(v1), glm::normalize(v2));
    return glm::acos(dot / (v1.length() * v2.length()));
}

bool TransformGizmo::update(glm::vec3 &new_position, const glm::vec3& controller_position, float delta)
{
	if (!enabled) {
		return false;
	}

	gizmo_position = new_position;

	if (!has_graved) {

        // POSITION GIZMO TESTS
		// Generate the AABB size of the bounding box of the arrow mesh
		// Center the pointa ccordingly, and add a bit of margin (0.01f) for
		// Grabing all axis in the bottom

        if (axis & GIZMO_AXIS_X) {
		    glm::vec3 size = position_gizmo_scale * glm::vec3(mesh_size.y, mesh_size.x, mesh_size.z);
		    glm::vec3 box_center = gizmo_position + glm::vec3(size.x / 2.0f - 0.01f, 0.0f, 0.0f);
            position_axis_x_selected = intersection::point_AABB(controller_position, box_center, size);
        }

        if (axis & GIZMO_AXIS_Y) {
            glm::vec3 size = position_gizmo_scale * mesh_size;
            glm::vec3 box_center = gizmo_position + glm::vec3(0.0f, size.y / 2.0f - 0.01f, 0.0f);
            position_axis_y_selected = intersection::point_AABB(controller_position, box_center, size);
        }

        if (axis & GIZMO_AXIS_Z) {
            glm::vec3 size = position_gizmo_scale * glm::vec3(mesh_size.x, mesh_size.z, mesh_size.y);
            glm::vec3 box_center = gizmo_position + glm::vec3(0.0f, 0.0f, size.z / 2.0f - 0.01f);
            position_axis_z_selected = intersection::point_AABB(controller_position, box_center, size);
        }

        const bool any_translation_grabed = position_axis_x_selected || position_axis_y_selected || position_axis_z_selected;

        // ROTATION GIZMO

        if (axis & GIZMO_AXIS_X) {
            // gneralize for all grabbing gestires
            rotation_axis_x_selected =  intersection::point_circle(controller_position, gizmo_position, glm::vec3(0.0f, 0.0f, 1.0f), rotation_gizmo_scale.x);
        }

        if (axis & GIZMO_AXIS_Y) {
            rotation_axis_y_selected = !rotation_axis_x_selected && intersection::point_circle(controller_position, gizmo_position, glm::vec3(1.0f, 0.0f, 0.0f), rotation_gizmo_scale.y);
        }

        if (axis & GIZMO_AXIS_Z) {
            rotation_axis_z_selected = !rotation_axis_y_selected && intersection::point_circle(controller_position, gizmo_position, glm::vec3(0.0f, 1.0f, 0.0f), rotation_gizmo_scale.z);
        }
	}

    const bool is_active = position_axis_x_selected || position_axis_y_selected || position_axis_z_selected || rotation_axis_x_selected || rotation_axis_y_selected || rotation_axis_z_selected;

	// Calculate the movement vector for the gizmo
	if (Input::get_trigger_value(HAND_RIGHT) > 0.3f) {

        glm::vec3 controller_delta = controller_position - prev_controller_position;

		if (has_graved) {

            if (type & POSITION_GIZMO) {
                glm::vec3 constraint = { 0.0f, 0.0f, 0.0f };
                if (position_axis_x_selected) {
                    constraint.x = 1.0f;
                }
                if (position_axis_y_selected) {
                    constraint.y = 1.0f;
                }
                if (position_axis_z_selected) {
                    constraint.z = 1.0f;
                }

                gizmo_position += controller_delta * constraint;
            }

            if (type & ROTATION_GIZMO) {
                x_angle = 0.0f, y_angle = 0.0f, z_angle = 0.0f;

                const glm::vec3 new_rotation_pose = controller_position;
                glm::vec3 t = new_rotation_pose - reference_rotation_pose;

                if (rotation_axis_x_selected) {
                    // x_angle = acos(dot(v.yz, u.yx)
                    x_angle = get_angle(glm::vec2(new_rotation_pose.y, new_rotation_pose.z),
                                        glm::vec2(reference_rotation_pose.y, reference_rotation_pose.z));
                }

                if (rotation_axis_y_selected) {
                    // y_angle = acos(dot(v.xz, u.xz)
                    y_angle = get_angle(glm::vec2(new_rotation_pose.x, new_rotation_pose.z),
                                        glm::vec2(reference_rotation_pose.x, reference_rotation_pose.z));
                }
                if (rotation_axis_z_selected) {
                    // z_angle = acos(dot(v.xy, u.xy)
                    z_angle = get_angle(glm::vec2(new_rotation_pose.x, new_rotation_pose.y),
                                        glm::vec2(reference_rotation_pose.x, reference_rotation_pose.y));
                }
                if (rotation_axis_x_selected || rotation_axis_y_selected || rotation_axis_z_selected) {
                    current_rotation =  glm::toQuat(glm::eulerAngleYXZ(y_angle, x_angle, z_angle));
                    int i =0;
                }
            }
        } else {
            // Stablish reference to rotation
            if (type & ROTATION_GIZMO) {
                reference_rotation_pose = controller_position;
            }
        }

		prev_controller_position = controller_position;
		has_graved = true;
	} else {
		has_graved = false;
	}

	new_position = gizmo_position;

    return is_active;
}

void TransformGizmo::render(int axis)
{
	if (!enabled) {
		return;
	}

	if (type & POSITION_GIZMO) {

        if (axis & GIZMO_AXIS_X)
        {
            arrow_mesh_x->set_translation(gizmo_position);
            arrow_mesh_x->scale(position_gizmo_scale);
		    arrow_mesh_x->rotate(glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));
		    arrow_mesh_x->set_surface_material_override_color(0, Color(1.f, 0.0f, 0.f, 1.f) + (position_axis_x_selected ? Color(0.5f, 0.5f, 0.5f, 0.f) : Color(0.f)));
		    arrow_mesh_x->render();
        }

        if (axis & GIZMO_AXIS_Y)
        {
		    arrow_mesh_y->set_translation(gizmo_position);
		    arrow_mesh_y->scale(position_gizmo_scale);
		    arrow_mesh_y->set_surface_material_override_color(0, Color(0.f, 1.f, 0.0f, 1.f) + (position_axis_y_selected ? Color(0.5f, 0.5f, 0.5f, 0.f) : Color(0.f)));
            arrow_mesh_y->render();
        }

        if (axis & GIZMO_AXIS_Z)
        {
            arrow_mesh_z->set_translation(gizmo_position);
            arrow_mesh_z->scale(position_gizmo_scale);
            arrow_mesh_z->rotate(glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
		    arrow_mesh_z->rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
		    arrow_mesh_z->set_surface_material_override_color(0, Color(0.0f, 0.f, 1.f, 1.f) + (position_axis_z_selected ? Color(0.5f, 0.5f, 0.5f, 0.f) : Color(0.f)));
		    arrow_mesh_z->render();
        }
	}

    if (type & ROTATION_GIZMO) {

        if (axis & GIZMO_AXIS_X)
        {
            wire_circle_mesh_x->set_translation(gizmo_position);
            wire_circle_mesh_x->scale(rotation_gizmo_scale * 0.5f);
            wire_circle_mesh_x->rotate(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
            wire_circle_mesh_x->set_surface_material_override_color(0, Color(1.f, 0.f, 0.f, 1.f) + (rotation_axis_x_selected ? Color(0.5f, 0.5f, 0.5f, 0.f) : Color(0.f)));
            wire_circle_mesh_x->render();
        }

        if (axis & GIZMO_AXIS_Y)
        {
            wire_circle_mesh_y->set_translation(gizmo_position);
            wire_circle_mesh_y->scale(rotation_gizmo_scale * 0.5f);
            wire_circle_mesh_y->rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
            wire_circle_mesh_y->set_surface_material_override_color(0, Color(0.f, 1.f, 0.f, 1.f) + (rotation_axis_z_selected ? Color(0.5f, 0.5f, 0.5f, 0.f) : Color(0.f)));
            wire_circle_mesh_y->render();
        }

        if (axis & GIZMO_AXIS_Z)
        {
            wire_circle_mesh_z->set_translation(gizmo_position);
            wire_circle_mesh_z->scale(rotation_gizmo_scale * 0.5f);
            wire_circle_mesh_z->set_surface_material_override_color(0, Color(0.f, 0.f, 1.f, 1.f) + (rotation_axis_y_selected ? Color(0.5f, 0.5f, 0.5f, 0.f) : Color(0.f)));
            wire_circle_mesh_z->render();
        }
    }
}
