#pragma once

#include "ik_solver.h"

class JacobianSolver : public IKSolver {

    std::vector<glm::vec3> local_axis_x_joint;
    std::vector<glm::vec3> local_axis_y_joint;
    std::vector<glm::vec3> local_axis_z_joint;

public:

    float amount = 0.05f;

    Transform& operator[](uint32_t index);

    void add_revolute_joint(const Transform& joint_transform);

    void set_chain(const std::vector<Transform>& chain);
    void set_rotation_axis();

    bool solve(const Transform& target);
};
