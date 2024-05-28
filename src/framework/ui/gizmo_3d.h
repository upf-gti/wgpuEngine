#pragma once

#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"
#include "framework/colors.h"

enum eGizmoAxis : uint8_t {
    GIZMO_AXIS_X = 1 << 0,
    GIZMO_AXIS_Y = 1 << 1,
    GIZMO_AXIS_Z = 1 << 2,
    GIZMO_AXIS_XZ = GIZMO_AXIS_X | GIZMO_AXIS_Z,
    GIZMO_ALL_AXIS = GIZMO_AXIS_X | GIZMO_AXIS_Y | GIZMO_AXIS_Z
};

enum eGizmoType : uint8_t {
    POSITION_GIZMO = 1 << 0,
    SCALE_GIZMO = 1 << 1,
    ROTATION_GIZMO = 1 << 2,
    SCALE_ROTATION_GIZMO = SCALE_GIZMO | ROTATION_GIZMO,
    POSITION_ROTATION_GIZMO = POSITION_GIZMO | ROTATION_GIZMO,
    POSITION_SCALE_ROTATION_GIZMO = POSITION_ROTATION_GIZMO | SCALE_GIZMO
};

class MeshInstance3D;

/*
	TRANSFORM GIZMO COMPONENT FOR VR
*/

class Gizmo3D {

    static Color X_AXIS_COLOR;
    static Color Y_AXIS_COLOR;
    static Color Z_AXIS_COLOR;

    static Color AXIS_SELECTED_OFFSET_COLOR;

    eGizmoType type = POSITION_GIZMO;
    eGizmoAxis axis = GIZMO_ALL_AXIS;

	bool enabled = true;
	bool has_graved = false;
    bool has_graved_position = false;
	
	MeshInstance3D* arrow_mesh_x = nullptr;
    MeshInstance3D* arrow_mesh_y = nullptr;
    MeshInstance3D* arrow_mesh_z = nullptr;

    MeshInstance3D* scale_arrow_mesh_x = nullptr;
    MeshInstance3D* scale_arrow_mesh_y = nullptr;
    MeshInstance3D* scale_arrow_mesh_z = nullptr;

    MeshInstance3D* wire_circle_mesh_x = nullptr;
    MeshInstance3D* wire_circle_mesh_y = nullptr;
    MeshInstance3D* wire_circle_mesh_z = nullptr;

    MeshInstance3D* free_hand_point_mesh = nullptr;

    void init_translation_meshes();
    void init_scale_meshes();
    void init_rotation_meshes();

	glm::vec3 prev_controller_position;
	glm::vec3 gizmo_position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 gizmo_scale = { 1.0f, 1.0f, 1.0f };

    glm::vec3 arrow_gizmo_scale = {};
	glm::vec3 mesh_size = {};

    glm::vec3 circle_gizmo_scale = {};
    glm::vec3 reference_rotation_pose;
    glm::quat current_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::quat rotation_diff = { 0.0f, 0.0f, 0.0f, 1.0f };

    bool free_hand_selected = false;

	bool position_axis_x_selected = false;
	bool position_axis_y_selected = false;
	bool position_axis_z_selected = false;

    bool scale_axis_x_selected = false;
    bool scale_axis_y_selected = false;
    bool scale_axis_z_selected = false;

    bool rotation_axis_x_selected = false;
    bool rotation_axis_y_selected = false;
    bool rotation_axis_z_selected = false;

    float x_angle = 0.0f;
    float y_angle = 0.0f;
    float z_angle = 0.0f;

public:

	void initialize(const eGizmoType& new_type, const glm::vec3 &position, const eGizmoAxis& new_axis = GIZMO_ALL_AXIS);
	void clean();

    void set_mode(const eGizmoType& gizmo_use, const eGizmoAxis& axis = GIZMO_ALL_AXIS);

	bool update(glm::vec3& new_position, const glm::vec3& controller_position, float delta_time);
    bool update(glm::vec3& new_position, glm::quat& rotation, const glm::vec3& controller_position, float delta_time);
    bool update(glm::vec3& new_position, glm::vec3& scale, const glm::vec3& controller_position, float delta_time);
    bool update(glm::vec3& new_position, glm::vec3& scale, glm::quat& rotation, const glm::vec3& controller_position, float delta_time);

    void render(int axis = GIZMO_ALL_AXIS);

    inline glm::quat  get_rotation() const {
        return current_rotation;
    }

    inline glm::vec3 get_euler_rotation() const {
        return glm::vec3(x_angle, y_angle, z_angle);
    }
};
