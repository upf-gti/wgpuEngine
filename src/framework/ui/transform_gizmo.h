#pragma once

#include "framework/math.h"

enum eGizmoAxis : uint8_t {
    GIZMO_AXIS_X = 1 << 0,
    GIZMO_AXIS_Y = 1 << 1,
    GIZMO_AXIS_Z = 1 << 2,
    GIZMO_ALL_AXIS = GIZMO_AXIS_X | GIZMO_AXIS_Y | GIZMO_AXIS_Z
};

enum eGizmoType : uint8_t {
	POSITION_GIZMO = 0b001,
	ROTATION_GIZMO = 0b010,
	POSITION_ROTATION_GIZMO = 0b011
};

class EntityMesh;

/*
	TRANSFORM GIZMO COMPONENT
	- Declare the kind of gizmo that you want (for now, only Position)
	- On update, give it the base position, and get the new position of
	  the gizmo, in order to use it for the parent.
*/

class TransformGizmo {

	eGizmoType  type;

	bool enabled = true;
	bool has_graved = false;
    bool has_graved_position = false;
	
	EntityMesh* arrow_mesh_x = nullptr;
    EntityMesh* arrow_mesh_y = nullptr;
    EntityMesh* arrow_mesh_z = nullptr;
    EntityMesh* wire_circle_mesh = nullptr;

	glm::vec3 prev_controller_position;
	glm::vec3 gizmo_position = {};
    glm::vec3 position_gizmo_scale = {};
    glm::vec3 rotation_gizmo_scale = {};
    glm::vec3 reference_rotation_pose;

	glm::vec3 mesh_size = { 0.300f, 1.7f, 0.300f };

    glm::quat current_rotation = {0.0f, 0.0f, 0.0f, 1.0f};

	bool position_axis_x_selected = false;
	bool position_axis_y_selected = false;
	bool position_axis_z_selected = false;

    bool rotation_axis_x_selected = false;
    bool rotation_axis_y_selected = false;
    bool rotation_axis_z_selected = false;

    float x_angle = 0.0f;
    float y_angle = 0.0f;
    float z_angle = 0.0f;

public:

	void initialize(const eGizmoType gizmo_use, const glm::vec3 &position);
	void clean();

	bool update(glm::vec3& new_position, const glm::vec3& controller_position, float delta);
    void render(int axis = GIZMO_ALL_AXIS);

    inline glm::quat  get_rotation() const {
        return current_rotation;
    }

    inline glm::vec3 get_euler_rotation() const {
        return glm::vec3(x_angle, y_angle, z_angle);
    }
};
