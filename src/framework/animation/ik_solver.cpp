#include "ik_solver.h"

IKSolver::IKSolver() {
    num_steps = 15;
    threshold = 0.00001f;
}

size_t IKSolver::size() {
    return ik_chain.size();
}

void IKSolver::resize(size_t newSize) {
    ik_chain.resize(newSize);
    joint_indices.resize(newSize);
    joint_constraint_type.resize(newSize);
    joint_constraint_value.resize(newSize);
}

Transform& IKSolver::operator[](uint32_t index) {
    return ik_chain[index];
}

std::vector<Transform>& IKSolver::get_chain() {
    return ik_chain;
}

std::vector<uint32_t> IKSolver::get_joint_indices() {
    return joint_indices;
}

void IKSolver::set_chain(std::vector<Transform> chain) {
    ik_chain = chain;
    resize(ik_chain.size());
}

void IKSolver::set_joint_indices(std::vector<uint32_t> indices) {
    joint_indices = indices;
}

Transform& IKSolver::get_local_transform(uint32_t index) {
    return ik_chain[index];
}

void IKSolver::set_parent_transform(const Transform& t) {
    aux_parent = t;
}

void IKSolver::set_local_transform(uint32_t index, const Transform& t) {
    ik_chain[index] = t;
}

void IKSolver::set_local_transform(uint32_t index, const Transform& t, uint32_t joint_idx) {
    ik_chain[index] = t;
    joint_indices[index] = joint_idx;
}

void IKSolver::set_global_transform(uint32_t index, const Transform& t) {
    // if its the root, just store
    if (index <= 0) {
        ik_chain[0] = t;
        return;
    }

    // convert to local to store
    Transform parentWorld = get_global_transform(index - 1);
    Transform local = Transform::combine(Transform::inverse(parentWorld), t);

    ik_chain[index] = local;
}

uint32_t IKSolver::get_num_steps() {
    return num_steps;
}

void IKSolver::set_num_steps(uint32_t steps) {
    num_steps = steps;
}

float IKSolver::get_threshold() {
    return threshold;
}

void IKSolver::set_threshold(float value) {
    threshold = value;
}

Transform IKSolver::get_global_transform(uint32_t index) {
    uint32_t size = (uint32_t)ik_chain.size();
    Transform world = ik_chain[index];
    for (int i = (int)index - 1; i >= 0; --i) {
        world = Transform::combine(ik_chain[i], world);
    }
    return world;
}

void IKSolver::set_ball_socket_constraint(uint32_t idx, float limit) {
    joint_constraint_type[idx] = BALL;
    joint_constraint_value[idx] = glm::vec3(limit);
}

void IKSolver::set_hinge_socket_constraint(uint32_t idx, glm::vec3 axis) {
    joint_constraint_type[idx] = HINGE;
    joint_constraint_value[idx] = axis;
}

void IKSolver::apply_ball_socket_constraint(int i, float limit) {
    glm::quat parent_rot = i == 0 ? aux_parent.get_rotation() : get_global_transform(i - 1).get_rotation();
    glm::quat this_rot = get_global_transform(i).get_rotation();

    glm::vec3 parentDir = parent_rot * glm::vec3(0, 0, 1);
    glm::vec3 thisDir = this_rot * glm::vec3(0, 0, 1);
    float a = Transform::get_angle_between_vectors(parentDir, thisDir);

    if (a > glm::radians(limit)) {
        glm::vec3 correction = cross(parentDir, thisDir);
        glm::quat worldSpaceRotation = parent_rot * angleAxis(glm::radians(limit), correction);
        if (i == 0) {
            ik_chain[i].set_rotation(worldSpaceRotation);
        }
        else {
            ik_chain[i].set_rotation(worldSpaceRotation * inverse(parent_rot));
        }
    }
}

void IKSolver::apply_hinge_socket_constraint(int i, glm::vec3 axis) {
    Transform joint = get_global_transform(i);
    Transform parent = i == 0 ? aux_parent : get_global_transform(i - 1);
    glm::vec3 current_hinge = joint.get_rotation() * axis;
    glm::vec3 desired_hinge = parent.get_rotation() * axis;
    ik_chain[i].set_rotation(ik_chain[i].get_rotation() * Transform::get_rotation_between_vectors(current_hinge, desired_hinge));
}
