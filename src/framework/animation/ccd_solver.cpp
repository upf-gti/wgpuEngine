#include "ccd_solver.h"

bool CCDSolver::solve(const Transform& target) {
    // Local variables and chain_size check
    unsigned int chain_size = size();

    if (chain_size == 0)
        return false;

    unsigned int last = chain_size - 1; // end effector
    float threshold_sq = threshold * threshold;
    glm::vec3 goal = target.position;

    for (unsigned int i = 0; i < num_steps; ++i) {
        // Check if we've reached the goal
        glm::vec3 effector = get_global_transform(last).position;
        if (glm::length2(goal - effector) < threshold_sq) {
            return true;
        }
        for (int j = (int)chain_size - 2; j >= 0; --j) {
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

            glm::quat world_rotated = rotation * effector_to_goal;
            glm::quat local_rotate = world_rotated * inverse(rotation);
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
} // End CCDSolver::Solve function

void CCDSolver::apply_ball_socket_constraint(int i, float limit) {
    glm::quat parent_rot = i == 0 ? aux_parent.rotation : get_global_transform(i - 1).rotation;
    glm::quat this_rot = get_global_transform(i).rotation;

    glm::vec3 parent_dir = parent_rot * glm::vec3(0, 0, 1);
    glm::vec3 this_dir = this_rot * glm::vec3(0, 0, 1);
    float angle = ::angle(parent_dir, this_dir);

    if (angle > glm::radians(limit)) {
        glm::vec3 correction = cross(parent_dir, this_dir);
        glm::quat world_space_rotation = parent_rot * angleAxis(glm::radians(limit), correction);
        if (i == 0) {
            ik_chain[i].rotation = world_space_rotation;
        }
        else {
            ik_chain[i].rotation = world_space_rotation * inverse(parent_rot);
        }
    }
}

void CCDSolver::apply_hinge_socket_constraint(int i, glm::vec3 axis) {
    // apply constraint to the joint (rotate the child)
    Transform joint = get_global_transform(i);
    Transform child = get_global_transform(i + 1);
    glm::vec3 desired_hinge = joint.rotation * axis;
    glm::vec3 current_hinge = child.rotation * axis;

    ik_chain[i + 1].rotation = ik_chain[i + 1].rotation * fromTo(current_hinge, desired_hinge);
}
