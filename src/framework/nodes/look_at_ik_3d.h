#pragma once

#include "framework/nodes/node_3d.h"
#include "framework/animation/skeleton.h"

#include <string>
#include <vector>

class SkeletonInstance3D;
class IKSolver;
class FABRIKSolver;

class LookAtIK3D : public Node
{
    uint32_t max_iterations = 15u;
    float min_distance = 0.1f;

    std::string root_name = "";
    std::string end_effector = "";

    Transform target;

    enum IKSolvers { FABRIK, CCD, JACOBIAN };
    uint32_t solver = IKSolvers::FABRIK;
    IKSolver* ik_solver = nullptr;

    SkeletonInstance3D* skeleton_instance = nullptr;

public:

    LookAtIK3D();
    LookAtIK3D(SkeletonInstance3D* new_skeleton_instance);

    ~LookAtIK3D() {};

    void set_iterations(uint32_t iterations);
    void set_distance(float distance);
    void set_root_name(const std::string& new_root_name) { root_name = new_root_name; }
    void set_end_effector(const std::string& new_end_effector) { end_effector = new_end_effector; }
    void set_solver(uint32_t solver_type);
    void set_target(Transform new_target) { target = new_target; };
    void set_skeleton_instance(SkeletonInstance3D* new_skeleton_instance) { skeleton_instance = new_skeleton_instance; };

    uint32_t get_iterations() { return max_iterations; }
    float get_distance() { return min_distance; }
    const std::string& get_root_name() { return root_name; }
    const std::string& get_end_effector() { return end_effector; };
    const Transform& get_target() { return target; }
    uint32_t get_solver() { return solver; }

    void update(float delta_time);
    void render_gui();

    void create_chain();
};
