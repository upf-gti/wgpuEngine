#include "fabrik_solver.h"
#include "glm/gtx/norm.hpp"

void FABRIKSolver::resize(size_t new_size) {
    ik_chain.resize(new_size);
    world_chain.resize(new_size);
    lengths.resize(new_size);
    joint_indices.resize(new_size);
    joint_constraint_type.resize(new_size);
    joint_constraint_value.resize(new_size);
}

void FABRIKSolver::ik_chain_to_world() {
    size_t chain_size = size();
    for (size_t i = 0; i < chain_size; ++i) {
        Transform world = get_global_transform(i);
        world_chain[i] = world.position;
        if (i >= 1) {
            glm::vec3 prev = world_chain[i - 1];
            lengths[i] = glm::length(world.position - prev);
        }
    }
    if (chain_size > 0) {
        lengths[0] = 0.0f;
    }
}

void FABRIKSolver::world_to_ik_chain() {
    size_t chain_size = size();
    if (chain_size == 0) { return; }
    for (int i = 0; i < chain_size - 1; ++i) {
        Transform world = get_global_transform(i);
        Transform next = get_global_transform(i + 1);
        glm::vec3 position = world.position;
        glm::quat rotation = world.rotation;

        glm::vec3 to_next = next.position - position;
        to_next = inverse(rotation) * to_next;
        glm::vec3 to_desired = world_chain[i + 1] - position;
        to_desired = inverse(rotation) * to_desired;

        glm::quat delta = fromTo(to_next, to_desired);
        ik_chain[i].rotation = ik_chain[i].rotation * delta;
    }
}

void FABRIKSolver::iterate_backward(const glm::vec3& goal) {
    size_t chain_size = size();
    if (chain_size > 0) {
        world_chain[chain_size - 1] = goal;
    }
    for (int i = chain_size - 2; i >= 0; --i) {
        glm::vec3 direction = normalize(world_chain[i] - world_chain[i + 1]);
        glm::vec3 offset = direction * lengths[i + 1];
        world_chain[i] = world_chain[i + 1] + offset;
    }
}

void FABRIKSolver::iterate_forward(const glm::vec3& base) {
    size_t chain_size = size();
    if (chain_size > 0) {
        world_chain[0] = base;
    }
    for (int i = 1; i < chain_size; ++i) {
        glm::vec3 direction = normalize(world_chain[i] - world_chain[i - 1]);
        glm::vec3 offset = direction * lengths[i];
        world_chain[i] = world_chain[i - 1] + offset;
    }
}

bool FABRIKSolver::solve(const Transform& target) {
    // Local variables and size check
    size_t chain_size = size();
    if (chain_size == 0) { return false; }
    uint32_t last = chain_size - 1;
    float thresholdSq = threshold * threshold;

    ik_chain_to_world();
    glm::vec3 goal = target.position;
    glm::vec3 base = world_chain[0];

    // [CA] To do:
    // For each iteration of the algorithm:
    for (int i = 0; i < num_steps; i++) {
        // 1. Check if the end-effector has reached the goal
        if (glm::length2(world_chain[last] - goal) <= thresholdSq) {
            world_to_ik_chain();
            return true;
        }
        // 2. Perform backward step
        iterate_backward(goal);
        // 3. Perform forward step
        iterate_forward(base);
        // 4. Apply constraints if required:
        // 4.1. Convert the chain in local space
        //world_to_ik_chain();
        // 4.2. Apply constraints

        // 4.3. Convert the chain in global space again
        //ik_chain_to_world();
    }
    // Convert the chain in local space again
    world_to_ik_chain();
    // Last check if end-effector has reached the goal
    if (glm::length2(world_chain[last] - goal) <= thresholdSq) {
        return true;
    }
    return false;
}
