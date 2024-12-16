#pragma once

#include "ik_solver.h"

class FABRIKSolver : public IKSolver {
protected:
    std::vector<glm::vec3> world_chain;
    std::vector<float> lengths;

    void ik_chain_to_world();
    void iterate_forward(const glm::vec3& goal);
    void iterate_backward(const glm::vec3& base);
    void world_to_ik_chain();

public:
    void resize(size_t new_size) override;
    bool solve(const Transform& target) override;
};
