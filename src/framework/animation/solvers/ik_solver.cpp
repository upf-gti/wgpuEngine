#include "ik_solver.h"

size_t IKSolver::size()
{
    return ik_chain.size();
}

void IKSolver::resize(size_t new_size)
{
    ik_chain.resize(new_size);
    joint_indices.resize(new_size);
    joint_constraint_type.resize(new_size);
    joint_constraint_value.resize(new_size);
}

Transform& IKSolver::operator[](uint32_t index)
{
    return ik_chain[index];
}

const Transform& IKSolver::get_joint(uint32_t idx)
{
    return ik_chain[idx];
}

void IKSolver::set_chain(std::vector<Transform> chain)
{
    ik_chain = chain;
    resize(ik_chain.size());
}

void IKSolver::set_joint_indices(std::vector<uint32_t> indices)
{
    joint_indices = indices;
}

Transform& IKSolver::get_local_transform(uint32_t index)
{
    return ik_chain[index];
}

void IKSolver::set_parent_transform(const Transform& t)
{
    aux_parent = t;
}

void IKSolver::set_local_transform(uint32_t index, const Transform& t)
{
    ik_chain[index] = t;
}

void IKSolver::set_local_transform(uint32_t index, const Transform& t, uint32_t joint_idx)
{
    ik_chain[index] = t;
    joint_indices[index] = joint_idx;
}

void IKSolver::set_global_transform(uint32_t index, const Transform& t)
{
    // if its the root, just store
    if (index <= 0) {
        ik_chain[0] = t;
        return;
    }

    // convert to local to store
    Transform parent_world = get_global_transform(index - 1);
    Transform local = Transform::combine(Transform::inverse(parent_world), t);

    ik_chain[index] = local;
}

Transform IKSolver::get_global_transform(uint32_t index)
{
    Transform world = ik_chain[index];
    for (int i = (int)index - 1; i >= 0; --i) {
        world = Transform::combine(ik_chain[i], world);
    }
    return world;
}

void IKSolver::set_ball_socket_constraint(uint32_t idx, float limit)
{
    joint_constraint_type[idx] = BALL;
    joint_constraint_value[idx] = glm::vec3(limit);
}

void IKSolver::set_hinge_socket_constraint(uint32_t idx, glm::vec3 axis)
{
    joint_constraint_type[idx] = HINGE;
    joint_constraint_value[idx] = axis;
}

void IKSolver::apply_ball_socket_constraint(int i, float limit_angle)
{
    glm::quat parent_rot = i == 0 ? aux_parent.get_rotation() : get_global_transform(i - 1).get_rotation();
    glm::quat this_rot = get_global_transform(i).get_rotation();

    glm::vec3 parent_dir = parent_rot * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 this_dir = this_rot * glm::vec3(0.0f, 0.0f, 1.0f);
    float angle = Transform::get_angle_between_vectors(parent_dir, this_dir);

    if (angle > glm::radians(limit_angle)) {
        glm::vec3 correction = glm::cross(parent_dir, this_dir);
        glm::quat world_rotation = parent_rot * glm::angleAxis(glm::radians(limit_angle), correction);
        if (i == 0) {
            ik_chain[i].set_rotation(world_rotation);
        }
        else {
            ik_chain[i].set_rotation(world_rotation * inverse(parent_rot));
        }
    }
}

void IKSolver::apply_hinge_socket_constraint(int i, glm::vec3 axis)
{
    Transform joint = get_global_transform(i);
    Transform parent = i == 0 ? aux_parent : get_global_transform(i - 1);
    glm::vec3 current_hinge = joint.get_rotation() * axis;
    glm::vec3 desired_hinge = parent.get_rotation() * axis;
    ik_chain[i].set_rotation(ik_chain[i].get_rotation() * Transform::get_rotation_between_vectors(current_hinge, desired_hinge));
}
