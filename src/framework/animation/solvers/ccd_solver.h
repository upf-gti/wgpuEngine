#pragma once

#include "ik_solver.h"

class CCDSolver : public IKSolver {
protected:

    Transform aux_parent; // in global

public:

    bool solve(const Transform& target) override;

    void apply_ball_socket_constraint(int i, float limit_angle);
    void apply_hinge_socket_constraint(int i, const glm::vec3& axis);
};
