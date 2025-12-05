#include "jacobian_solver.h"

#include "glm/gtx/norm.hpp"

Transform& JacobianSolver::operator[](uint32_t index)
{
    return ik_chain[index];
}

void JacobianSolver::add_revolute_joint(const Transform& joint_transform)
{
    ik_chain.push_back(joint_transform);
}

void JacobianSolver::set_chain(const std::vector<Transform>& chain)
{
    ik_chain = chain;
    resize(ik_chain.size());
    set_rotation_axis();
}

void JacobianSolver::set_rotation_axis()
{
    glm::vec3 local_axis = glm::vec3(0.0f);

    for (uint32_t i = 0u; i < ik_chain.size(); ++i) {

        const Transform& t = get_global_transform(i);
        const glm::quat& inv_rot = glm::inverse(t.get_rotation());

        local_axis = inv_rot * glm::vec3(1.f, 0.f, 0.f);
        local_axis_x_joint.push_back(local_axis);

        local_axis = inv_rot * glm::vec3(0.f, 1.f, 0.f);
        local_axis_y_joint.push_back(local_axis);

        local_axis = inv_rot * glm::vec3(0.f, 0.f, 1.f);
        local_axis_z_joint.push_back(local_axis);
    }
}

bool JacobianSolver::solve(const Transform& target)
{
    uint32_t num_joints = static_cast<uint32_t>(ik_chain.size()) - 1u; // we don't need to compute the rotations for the end effector jont
    glm::vec3 goal_pos = target.get_position();

    // init the jacobi matrix (DoF * rotation axis)
    std::vector<float> jacobi_mat((num_joints * 3u) * 3u); // nCol = num_joints * 3 (each joint rotates for each axis XYZ separately) and nRow = 3 (3 components)

    for (uint32_t i = 0u; i < num_steps; ++i) {

        // get end effector position
        glm::vec3 effector_pos = get_global_transform(num_joints).get_position();

        // get the differential translation
        glm::vec3 diff_pos = goal_pos - effector_pos;

        if (glm::length2(diff_pos) < threshold * threshold) {
            return true;
        }

        // build jacobi matrix as an array
        for (uint32_t joint_idx = 0; joint_idx < num_joints; joint_idx++) {
            // position of current joint in global
            Transform joint_transform = get_global_transform(joint_idx);
            glm::vec3 joint_pos = joint_transform.get_position();
            glm::quat joint_rot = joint_transform.get_rotation();

            // vector from joint to end effector and from joint to target
            glm::vec3 to_effector = effector_pos - joint_pos;
            
            // compute rotation axis
            glm::vec3 joint_axis_x = local_axis_x_joint[joint_idx];
            joint_axis_x = joint_rot * joint_axis_x;

            glm::vec3 joint_axis_y = local_axis_y_joint[joint_idx];
            joint_axis_y = joint_rot * joint_axis_y;

            glm::vec3 joint_axis_z = local_axis_z_joint[joint_idx];
            joint_axis_z = joint_rot * joint_axis_z;

            glm::vec3 cross_prod_x = cross(joint_axis_x, to_effector);
            glm::vec3 cross_prod_y = cross(joint_axis_y, to_effector);
            glm::vec3 cross_prod_z = cross(joint_axis_z, to_effector);

            // Column Major, as our mat class
            jacobi_mat[joint_idx * 9] = cross_prod_x.x;
            jacobi_mat[(joint_idx * 9) + 1] = cross_prod_x.y; // joint idx * nº axis per joint + each component position
            jacobi_mat[(joint_idx * 9) + 2] = cross_prod_x.z;

            jacobi_mat[(joint_idx * 9) + 3] = cross_prod_y.x;
            jacobi_mat[(joint_idx * 9) + 4] = cross_prod_y.y;
            jacobi_mat[(joint_idx * 9) + 5] = cross_prod_y.z;

            jacobi_mat[(joint_idx * 9) + 6] = cross_prod_z.x;
            jacobi_mat[(joint_idx * 9) + 7] = cross_prod_z.y;
            jacobi_mat[(joint_idx * 9) + 8] = cross_prod_z.z;
        }

        // transpose the jacobian (TODO: compute the pseudoinverse)
        std::vector<float> transposed_jacobi_mat(jacobi_mat.size());
        uint32_t idx = 0;
        for (uint32_t i_axis = 0; i_axis < 3; i_axis++) {
            for (uint32_t i_joint = 0; i_joint < num_joints; i_joint++) {
                transposed_jacobi_mat[idx] = jacobi_mat[i_axis + (i_joint * 9)];
                transposed_jacobi_mat[idx + 1] = jacobi_mat[i_axis + (i_joint * 9) + 3];
                transposed_jacobi_mat[idx + 2] = jacobi_mat[i_axis + (i_joint * 9) + 6];
                idx += 3;
            }
        }

        // find the differential rotation using: J^-1 * diff(pos) = diff(rot)
        std::vector<float> diff_rot(num_joints * 3);
        for (uint32_t i = 0; i < diff_rot.size(); i++) {
            diff_rot[i] = jacobi_mat[i * 3] * diff_pos.x + jacobi_mat[(i * 3) + 1] * diff_pos.y + jacobi_mat[(i * 3) + 2] * diff_pos.z;
        }

        // apply the new rotations (using a little step)
        for (uint32_t joint_idx = 0; joint_idx < num_joints; joint_idx++) {
            Transform joint_transform = get_global_transform(joint_idx);
            // create auxiliary glm::quats to rotate
            glm::quat qx = angleAxis(diff_rot[(joint_idx * 3)] * amount, local_axis_x_joint[joint_idx]);
            glm::quat qy = angleAxis(diff_rot[(joint_idx * 3) + 1] * amount, local_axis_y_joint[joint_idx]);
            glm::quat qz = angleAxis(diff_rot[(joint_idx * 3) + 2] * amount, local_axis_z_joint[joint_idx]);

            // combine all the rotations for each axis of the same joint
            glm::quat q = qz * qy * qx;
            Transform new_joint_transform(glm::vec3(0.0f), q, glm::vec3(1.0));
            new_joint_transform = Transform::combine(joint_transform, new_joint_transform);
            set_global_transform(joint_idx, new_joint_transform);
        }
    }

    return false;
}
