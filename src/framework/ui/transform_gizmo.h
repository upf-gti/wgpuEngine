#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

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

	eGizmoType				type;
	bool					enabled = true;
	
	EntityMesh*				arrow_mesh_x = nullptr;
    EntityMesh*             arrow_mesh_y = nullptr;
    EntityMesh*             arrow_mesh_z = nullptr;
    EntityMesh*             wire_circle_mesh = nullptr;

	glm::vec3				prev_controller_position;
	bool					has_graved = false;
    bool                    has_graved_position = false;

	glm::vec3               gizmo_position = {};
    glm::vec3               position_gizmo_scale = {};
    glm::vec3               rotation_gizmo_scale = {};

	glm::vec3               mesh_size = { 0.300f, 1.7f, 0.300f };

	bool                    position_axis_x_selected = false;
	bool                    position_axis_y_selected = false;
	bool                    position_axis_z_selected = false;

    bool                    rotation_axis_x_selected = false;
    bool                    rotation_axis_y_selected = false;
    bool                    rotation_axis_z_selected = false;

    glm::vec3               reference_rotation_pose;

    glm::quat               current_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    float                   x_angle = 0.0f;
    float                   y_angle = 0.0f;
    float                   z_angle = 0.0f;
public:

	void			initialize(const eGizmoType gizmo_use, const glm::vec3 &position);
	void			clean();

	glm::vec3		update(const glm::vec3& new_position, const float delta);
	void			render();



    inline glm::quat  get_rotation() const {
        return current_rotation;
    }

    inline glm::vec3 get_euler_rotation() const {
        return glm::vec3(x_angle, y_angle, z_angle);
    }
};
