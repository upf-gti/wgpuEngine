#pragma once

#include "framework/nodes/node_3d.h"
#include "framework/animation/fabrik_solver.h"
#include "framework/animation/ccd_solver.h"
#include "framework/animation/jacobian_solver.h"
#include "framework/animation/skeleton.h"
#include "framework/nodes/skeleton_instance_3d.h"

#include <string>
#include <vector>


class LookAtIK3D : public Node
{
    int max_iterations = 15;
    float min_distance = 0.1f;

    std::string root = "";
    std::string end_effector = "";

    Transform target;

    enum IKSolvers { FABRIK, CCD, JACOBIAN };
    int solver = IKSolvers::FABRIK;
    IKSolver* ik_solver = new FABRIKSolver();

    SkeletonInstance3D* skeleton_instance = nullptr;

public:
    LookAtIK3D() {
        ik_solver->set_num_steps(max_iterations);
        ik_solver->set_threshold(min_distance);
    };
    LookAtIK3D(SkeletonInstance3D* in_skeleton_instance);

    virtual ~LookAtIK3D() {};

    void set_iterations(uint32_t iterations);
    void set_distance(uint32_t distance);
    void set_root(const std::string& joint_name);
    void set_end_effector(const std::string& joint_name);
    void set_target(Transform transform);
    void set_solver(int solver_type);
    void set_skeleton_instance(SkeletonInstance3D* new_skeleton_instance);

    uint32_t get_iterations();
    uint32_t get_distance();
    const std::string& get_root();
    const std::string& get_end_effector();
    const Transform& get_target();
    int get_solver();

    void update(float delta_time);
    void render_gui();

    void create_chain();

};
