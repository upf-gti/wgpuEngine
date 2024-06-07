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
    glm::vec3 goal = target.position;

    for (uint32_t i = 0; i < num_steps; ++i) {
        // Check if we've reached the goal
        glm::vec3 effector = get_global_transform(last).position;
        if (glm::length2(goal - effector) < threshold_sq) {
            return true;
        }
        for (uint32_t j = (uint32_t)chain_size - 2u; j >= 0u; --j) {
            effector = get_global_transform(last).position;

            Transform world = get_global_transform(j);
            glm::vec3 position = world.position;
            glm::quat rotation = world.rotation;

            glm::vec3 to_effector = effector - position;
            glm::vec3 to_goal = goal - position;

            glm::quat effector_to_goal;
            if (glm::length2(to_goal) > 0.00001f) {
                effector_to_goal = fromTo(to_effector, to_goal);
            }

            glm::quat world_rotated = effector_to_goal * rotation;
            glm::quat local_rotate = inverse(rotation) * world_rotated;
            ik_chain[j].rotation = local_rotate * ik_chain[j].rotation;

            // -> APPLY CONSTRAINTS HERE!
            if (joint_constraint_type[j] == BALL) {
                apply_ball_socket_constraint(j, joint_constraint_value[j].x);
            }
            else if (joint_constraint_type[j] == HINGE) {
                apply_hinge_socket_constraint(j, joint_constraint_value[j]);
            }

            effector = get_global_transform(last).position;

            if (glm::length2(goal - effector) < threshold_sq) {
                return true;
            }
        }
    }

    return false;
}

void CCDSolver::apply_ball_socket_constraint(int i, float limit)
{
    glm::quat parent_rot = i == 0 ? aux_parent.rotation : get_global_transform(i - 1).rotation;
    glm::quat this_rot = get_global_transform(i).rotation;

    glm::vec3 parent_dir = parent_rot * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 this_dir = this_rot * glm::vec3(0.0f, 0.0f, 1.0f);
    float angle = ::angle(parent_dir, this_dir);

    if (angle > glm::radians(limit)) {
        glm::vec3 correction = cross(parent_dir, this_dir);
        glm::quat world_space_rotation = parent_rot * angleAxis(glm::radians(limit), correction);
        if (i == 0) {
            ik_chain[i].rotation = world_space_rotation;
        }
        else {
            ik_chain[i].rotation = inverse(parent_rot) * world_space_rotation;
        }
    }
}

void CCDSolver::apply_hinge_socket_constraint(int i, const glm::vec3& axis)
{
    // apply constraint to the joint (rotate the child)
    Transform joint = get_global_transform(i);
    Transform child = get_global_transform(i + 1);
    glm::vec3 desired_hinge = joint.rotation * axis;
    glm::vec3 current_hinge = child.rotation * axis;

    ik_chain[i + 1].rotation = fromTo(current_hinge, desired_hinge) * ik_chain[i + 1].rotation;
}
