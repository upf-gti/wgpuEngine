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
    std::string name = "Skeleton3DIK";

    uint32_t max_iterations = 15;
    float min_distance = 0.1f;

    std::string root = "";
    std::string end_effector = "";

    Transform target;

    enum IKSolvers { FABRIK, CCD, JACOBIAN };
    uint32_t solver = IKSolvers::FABRIK;
    IKSolver* ik_solver = nullptr;

    SkeletonInstance3D* skeleton_instance = nullptr;

public:

    LookAtIK3D();
    LookAtIK3D(SkeletonInstance3D* in_skeleton_instance);

    virtual ~LookAtIK3D() {};

    void set_iterations(uint32_t iterations);
    void set_distance(float& distance);
    void set_root(const std::string& joint_name);
    void set_end_effector(const std::string& joint_name);
    void set_target(Transform transform);
    void set_solver(uint32_t solver_type);
    void set_skeleton_instance(SkeletonInstance3D* new_skeleton_instance);

    uint32_t get_iterations();
    float get_distance();
    const std::string& get_root();
    const std::string& get_end_effector();
    const Transform& get_target();
    uint32_t get_solver();

    void update(float delta_time);
    void render_gui();

    void create_chain();

};
