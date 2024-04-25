#pragma once

#include "iK_solver.h"
#include "bone_transform.h"
#include <vector>

class JacobianSolver : public IKSolver {
protected:
    std::vector<glm::vec3> local_axis_x_joint;
    std::vector<glm::vec3> local_axis_y_joint;
    std::vector<glm::vec3> local_axis_z_joint;

public:
    float amount = 0.05;
    Transform& operator[](uint32_t index);

    void add_revolute_joint(Transform T);

    Transform get_joint(uint32_t idx);
    std::vector<Transform> get_chain();

    void set_chain(std::vector<Transform> chain);
    void set_rotation_axis();

    bool solve(const Transform& target);
};
