#pragma once

#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"

#include "gizmo_2d.h"
#include "framework/colors.h"
#include "framework/math/transform.h"

// From ImGuizmo:
enum eGizmoType {
    TRANSLATE_X = (1u << 0),
    TRANSLATE_Y = (1u << 1),
    TRANSLATE_Z = (1u << 2),
    ROTATE_X = (1u << 3),
    ROTATE_Y = (1u << 4),
    ROTATE_Z = (1u << 5),
    ROTATE_SCREEN = (1u << 6),
    SCALE_X = (1u << 7),
    SCALE_Y = (1u << 8),
    SCALE_Z = (1u << 9),
    BOUNDS = (1u << 10),
    SCALE_XU = (1u << 11),
    SCALE_YU = (1u << 12),
    SCALE_ZU = (1u << 13),

    TRANSLATE = TRANSLATE_X | TRANSLATE_Y | TRANSLATE_Z,
    ROTATE = ROTATE_X | ROTATE_Y | ROTATE_Z | ROTATE_SCREEN,
    SCALE = SCALE_X | SCALE_Y | SCALE_Z,
    SCALEU = SCALE_XU | SCALE_YU | SCALE_ZU, // universal
    UNIVERSAL = TRANSLATE | ROTATE | SCALEU
};

inline eGizmoType operator|(eGizmoType lhs, eGizmoType rhs) {
    return static_cast<eGizmoType>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

class MeshInstance3D;

/*
	TRANSFORM GIZMO COMPONENT
*/

class Gizmo3D {

    static Color X_AXIS_COLOR;
    static Color Y_AXIS_COLOR;
    static Color Z_AXIS_COLOR;

    static Color AXIS_SELECTED_OFFSET_COLOR;
    static Color AXIS_NOT_SELECTED_COLOR;

    eGizmoType operation = TRANSLATE;

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

	void initialize(const eGizmoType& new_type, const glm::vec3 &position = glm::vec3(0.0f));
	void clean();

    void set_operation(const eGizmoType& gizmo_use);
    void set_transform(const Transform& t);

	bool update(glm::vec3& new_position, const glm::vec3& controller_position, float delta_time);
    bool update(glm::vec3& new_position, glm::quat& rotation, const glm::vec3& controller_position, float delta_time);
    bool update(glm::vec3& new_position, glm::vec3& scale, const glm::vec3& controller_position, float delta_time);
    bool update(Transform& t, const glm::vec3& controller_position, float delta_time);
    bool update(const glm::vec3& controller_position, float delta_time);

    void render();
    void render_xr();

    Transform& get_transform() { return transform; };
    inline glm::vec3 get_position() const { return transform.get_position(); }
    inline glm::quat get_rotation() const { return transform.get_rotation(); }
    inline glm::vec3 get_euler_rotation() const { return glm::vec3(x_angle, y_angle, z_angle); }
};
