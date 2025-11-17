#include "fabrik_solver.h"

#include "glm/gtx/norm.hpp"

void FABRIKSolver::resize(size_t new_size)
{
    IKSolver::resize(new_size);

    world_chain.resize(new_size);
    lengths.resize(new_size);
}

void FABRIKSolver::ik_chain_to_world()
{
    uint32_t chain_size = static_cast<uint32_t>(size());
    for (uint32_t i = 0; i < chain_size; ++i) {
        Transform world = get_global_transform(i);
        world_chain[i] = world.get_position();
        if (i >= 1) {
            glm::vec3 prev = world_chain[i - 1];
            lengths[i] = glm::length(world.get_position() - prev);
        }
    }
    if (chain_size > 0) {
        lengths[0] = 0.0f;
    }
}

void FABRIKSolver::world_to_ik_chain()
{
    uint32_t chain_size = static_cast<uint32_t>(size());

    if (chain_size == 0) {
        return;
    }

    for (uint32_t i = 0; i < chain_size - 1; ++i) {
        Transform world = get_global_transform(i);
        Transform next = get_global_transform(i + 1);
        glm::vec3 position = world.get_position();
        glm::quat rotation = world.get_rotation();

        glm::vec3 to_next = next.get_position() - position;
        to_next = inverse(rotation) * to_next;
        glm::vec3 to_desired = world_chain[i + 1] - position;
        to_desired = inverse(rotation) * to_desired;

        glm::quat delta = Transform::get_rotation_between_vectors(to_next, to_desired);
        ik_chain[i].set_rotation(ik_chain[i].get_rotation() * delta);
    }
}

void FABRIKSolver::iterate_backward(const glm::vec3& goal)
{
    uint32_t chain_size = static_cast<uint32_t>(size());
    if (chain_size > 0) {
        world_chain[chain_size - 1] = goal;
    }
    for (uint32_t i = chain_size - 2u; i != (uint32_t)-1; --i) {
        glm::vec3 direction = normalize(world_chain[i] - world_chain[i + 1]);
        glm::vec3 offset = direction * lengths[i + 1];
        world_chain[i] = world_chain[i + 1] + offset;
    }
}

void FABRIKSolver::iterate_forward(const glm::vec3& base)
{
    size_t chain_size = size();
    if (chain_size > 0) {
        world_chain[0] = base;
    }
    for (size_t i = 1; i < chain_size; ++i) {
        glm::vec3 direction = normalize(world_chain[i] - world_chain[i - 1]);
        glm::vec3 offset = direction * lengths[i];
        world_chain[i] = world_chain[i - 1] + offset;
    }
}

bool FABRIKSolver::solve(const Transform& target)
{
    // Local variables and size check
    uint32_t chain_size = static_cast<uint32_t>(size());
    if (chain_size == 0) {
        return false;
    }

    uint32_t last = chain_size - 1u;
    float threshold_sq = threshold * threshold;

    ik_chain_to_world();

    glm::vec3 goal = target.get_position();
    glm::vec3 base = world_chain[0];

    // [CA] To do:
    // For each iteration of the algorithm:
    for (uint32_t i = 0; i < num_steps; i++) {

        const glm::vec3& effector= world_chain[last];
        if (glm::length2(goal - effector) < threshold_sq) {
            world_to_ik_chain();
            return true;
        }

        iterate_backward(goal);
        iterate_forward(base);

        world_to_ik_chain();

        ik_chain_to_world();
    }

    // Convert the chain in local space again
    world_to_ik_chain();

    const glm::vec3& effector = get_global_transform(last).get_position();
    if (glm::length2(goal - effector) < threshold_sq) {
        return true;
    }

    return false;
}
