#pragma once

#include "gizmo_2d.h"
#include "framework/colors.h"
#include "framework/math/transform.h"

using namespace ImGuizmo;
using eGizmoOp = OPERATION;

class MeshInstance3D;

/*
	TRANSFORM GIZMO COMPONENT
*/

class Gizmo3D {
#ifdef __EMSCRIPTEN__
public:
#endif

    static Color X_AXIS_COLOR;
    static Color Y_AXIS_COLOR;
    static Color Z_AXIS_COLOR;

    static Color AXIS_SELECTED_OFFSET_COLOR;
    static Color AXIS_NOT_SELECTED_COLOR;

    eGizmoOp operation = TRANSLATE;

	bool enabled = true;
	bool xr_enabled = false;
	bool has_graved = false;
    bool has_graved_position = false;
	
	MeshInstance3D* arrow_mesh_x = nullptr;
    MeshInstance3D* arrow_mesh_y = nullptr;
    MeshInstance3D* arrow_mesh_z = nullptr;

    MeshInstance3D* scale_arrow_mesh_x = nullptr;
    MeshInstance3D* scale_arrow_mesh_y = nullptr;
    MeshInstance3D* scale_arrow_mesh_z = nullptr;

    MeshInstance3D* scale_sphere_mesh_x = nullptr;
    MeshInstance3D* scale_sphere_mesh_y = nullptr;
    MeshInstance3D* scale_sphere_mesh_z = nullptr;

    MeshInstance3D* wire_circle_mesh_x = nullptr;
    MeshInstance3D* wire_circle_mesh_y = nullptr;
    MeshInstance3D* wire_circle_mesh_z = nullptr;

    MeshInstance3D* free_hand_point_mesh = nullptr;

    void init_translation_meshes();
    void init_scale_meshes();
    void init_rotation_meshes();

    Transform transform;

    const glm::vec3 arrow_gizmo_scale = glm::vec3(0.1f);
    const glm::vec3 sphere_gizmo_scale = glm::vec3(0.1f);
	const glm::vec3 mesh_size = glm::vec3(0.3f, 1.8f, 0.3f);

    const float circle_gizmo_scale = 0.1f;

    glm::quat last_hand_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::vec3 last_hand_translation = {};
    glm::vec3 start_hand_translation = {};

    bool free_hand_selected = false;

    glm::bvec3 position_axis_selected = glm::bvec3(false);
    glm::bvec3 scale_axis_selected = glm::bvec3(false);
    glm::bvec3 rotation_axis_selected = glm::bvec3(false);

    float x_angle = 0.0f;
    float y_angle = 0.0f;
    float z_angle = 0.0f;

    // 2D mode when no VR
    Gizmo2D gizmo_2d;

public:

	void initialize(const eGizmoOp& new_type, const glm::vec3 &position = glm::vec3(0.0f));
	void clean();

    void set_enabled(bool enabled);
    void set_operation(const eGizmoOp& gizmo_use);
    void set_transform(const Transform& t);

	bool update(glm::vec3& new_position, const glm::vec3& controller_position, float delta_time);
    bool update(glm::vec3& new_position, glm::quat& rotation, const glm::vec3& controller_position, float delta_time);
    bool update(glm::vec3& new_position, glm::vec3& scale, const glm::vec3& controller_position, float delta_time);
    bool update(Transform& t, const glm::vec3& controller_position, float delta_time);
    bool update(const glm::vec3& controller_position, float delta_time);

    // This returns bool due to the gizmo2d being used..
    bool render();
    void render_xr();

    Transform& get_transform() { return transform; };
    inline glm::vec3 get_position() const { return transform.get_position(); }
    inline glm::quat get_rotation() const { return transform.get_rotation(); }
    inline glm::vec3 get_euler_rotation() const { return glm::vec3(x_angle, y_angle, z_angle); }
};
