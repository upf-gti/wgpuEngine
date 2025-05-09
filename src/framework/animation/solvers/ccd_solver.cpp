#include "ccd_solver.h"

#include "glm/gtx/norm.hpp"

bool CCDSolver::solve(const Transform& target)
{
    // Local variables and chain_size check
    size_t chain_size = size();

    if (chain_size == 0u) {
        return false;
    }

    uint32_t last = chain_size - 1u; // end effector
    float threshold_sq = threshold * threshold;
    const glm::vec3& goal = target.get_position();

    for (uint32_t i = 0; i < num_steps; ++i) {
        // Check if we've reached the goal
        glm::vec3 effector = get_global_transform(last).get_position();
        if (glm::length2(goal - effector) < threshold_sq) {
            return true;
        }
        for (int j = chain_size - 2; j >= 0; --j) {
            effector = get_global_transform(last).get_position();

            const Transform world = get_global_transform(j);
            const glm::vec3& position = world.get_position();
            const glm::quat& rotation = world.get_rotation();

            const glm::vec3 to_effector = effector - position;
            const glm::vec3 to_goal = goal - position;

            glm::quat effector_to_goal;
            if (glm::length2(to_goal) > 0.00001f) {
                effector_to_goal = Transform::get_rotation_between_vectors(to_effector, to_goal);
            }

            const glm::quat world_rotated = effector_to_goal * rotation;
            const glm::quat local_rotate = inverse(rotation) * world_rotated;
            ik_chain[j].set_rotation(local_rotate * ik_chain[j].get_rotation());

            // -> APPLY CONSTRAINTS HERE!
            if (joint_constraint_type[j] == BALL) {
                apply_ball_socket_constraint(j, joint_constraint_value[j].x);
            }
            else if (joint_constraint_type[j] == HINGE) {
                apply_hinge_socket_constraint(j, joint_constraint_value[j]);
            }

            effector = get_global_transform(last).get_position();

            if (glm::length2(goal - effector) < threshold_sq) {
                return true;
            }
        }
    }

    return false;
}

void CCDSolver::apply_ball_socket_constraint(int i, float limit_angle)
{
    const glm::quat parent_rot = i == 0 ? aux_parent.get_rotation() : get_global_transform(i - 1).get_rotation();
    const glm::quat this_rot = get_global_transform(i).get_rotation();

    const glm::vec3 parent_dir = parent_rot * glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 this_dir = this_rot * glm::vec3(0.0f, 0.0f, 1.0f);
    float angle = Transform::get_angle_between_vectors(parent_dir, this_dir);

    if (angle > glm::radians(limit_angle)) {
        const glm::vec3 correction = glm::cross(parent_dir, this_dir);
        const glm::quat world_space_rotation = parent_rot * glm::angleAxis(glm::radians(limit_angle), correction);
        if (i == 0) {
            ik_chain[i].set_rotation(world_space_rotation);
        }
        else {
            ik_chain[i].set_rotation(inverse(parent_rot) * world_space_rotation);
        }
    }
}

void CCDSolver::apply_hinge_socket_constraint(int i, const glm::vec3& axis)
{
    // apply constraint to the joint (rotate the child)
    const Transform joint = get_global_transform(i);
    const Transform child = get_global_transform(i + 1);
    const glm::vec3 desired_hinge = joint.get_rotation() * axis;
    const glm::vec3 current_hinge = child.get_rotation() * axis;

    ik_chain[i + 1].set_rotation(Transform::get_rotation_between_vectors(current_hinge, desired_hinge) * ik_chain[i + 1].get_rotation());
}
