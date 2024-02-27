#pragma once

#include "framework/math.h"

enum eGizmoAxis : uint8_t {
    GIZMO_AXIS_X = 1 << 0,
    GIZMO_AXIS_Y = 1 << 1,
    GIZMO_AXIS_Z = 1 << 2,
    GIZMO_AXIS_XZ = GIZMO_AXIS_X | GIZMO_AXIS_Z,
    GIZMO_ALL_AXIS = GIZMO_AXIS_X | GIZMO_AXIS_Y | GIZMO_AXIS_Z
};

enum eGizmoType : uint8_t {
	POSITION_GIZMO = 1 << 0,
	ROTATION_GIZMO = 1 << 1,
	POSITION_ROTATION_GIZMO = POSITION_GIZMO | ROTATION_GIZMO
};

class EntityMesh;

/*
	TRANSFORM GIZMO COMPONENT
	- Declare the kind of gizmo that you want (for now, only Position)
	- On update, give it the base position, and get the new position of
	  the gizmo, in order to use it for the parent.
*/

class TransformGizmo {

	eGizmoType type;
    eGizmoAxis axis;

	bool enabled = true;
	bool has_graved = false;
    bool has_graved_position = false;
	
	EntityMesh* arrow_mesh_x = nullptr;
    EntityMesh* arrow_mesh_y = nullptr;
    EntityMesh* arrow_mesh_z = nullptr;

    EntityMesh* wire_circle_mesh_x = nullptr;
    EntityMesh* wire_circle_mesh_y = nullptr;
    EntityMesh* wire_circle_mesh_z = nullptr;

    void init_arrow_meshes();
    void init_circle_meshes();

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

	void initialize(const eGizmoType& gizmo_type, const glm::vec3 &position, const eGizmoAxis& axis = GIZMO_ALL_AXIS);
	void clean();

    void set_mode(const eGizmoType& gizmo_use, const eGizmoAxis& axis = GIZMO_ALL_AXIS);

	bool update(glm::vec3& new_position, const glm::vec3& controller_position, float delta);

    bool update(glm::vec3& new_position, glm::quat& rotation, const glm::vec3& controller_position, float delta) {
        bool result = update(new_position, controller_position, delta);
        rotation = current_rotation;
        return result;
    }

    void render(int axis = GIZMO_ALL_AXIS);

    inline glm::quat  get_rotation() const {
        return current_rotation;
    }

    inline glm::vec3 get_euler_rotation() const {
        return glm::vec3(x_angle, y_angle, z_angle);
    }
};
