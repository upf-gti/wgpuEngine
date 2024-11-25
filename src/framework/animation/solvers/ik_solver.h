#pragma once

#include "framework/math/transform.h"

#include <vector>

class IKSolver {
protected:

    Transform aux_parent; // in global
    std::vector<Transform> ik_chain;
    std::vector<uint32_t> joint_indices;
    std::vector<unsigned int> joint_constraint_type; // type
    std::vector<glm::vec3> joint_constraint_value; // limit or axis (in case is limit, set vec3 as (limit, limit, limit))

    uint32_t num_steps = 16u;
    float threshold = 0.00001f;

public:

    enum constraints {
        BALL = 1,
        HINGE
    };

    IKSolver() {}

    size_t size();
    virtual void resize(size_t new_size);
    Transform& operator[](uint32_t index);

    std::vector<Transform>& get_chain() { return ik_chain; }
    const std::vector<uint32_t>& get_joint_indices() { return joint_indices; }
    uint32_t get_num_steps() { return num_steps; }
    float get_threshold() { return threshold; }
    const Transform& get_joint(uint32_t idx);
    Transform& get_local_transform(uint32_t index);
    Transform get_global_transform(uint32_t index);

    void set_chain(std::vector<Transform> chain);
    void set_joint_indices(std::vector<uint32_t> indices);
    void set_parent_transform(const Transform& t);
    void set_local_transform(uint32_t index, const Transform& t);
    void set_local_transform(uint32_t index, const Transform& t, uint32_t joint_idx);
    void set_global_transform(uint32_t index, const Transform& t);
    void set_num_steps(uint32_t new_num_steps) { num_steps = new_num_steps; }
    void set_threshold(float new_threshold) { threshold = new_threshold; }
    void set_ball_socket_constraint(uint32_t idx, float limit);
    void set_hinge_socket_constraint(uint32_t idx, glm::vec3 axis);

    void apply_ball_socket_constraint(int i, float limit_angle);
    void apply_hinge_socket_constraint(int i, glm::vec3 axis);

    virtual bool solve(const Transform& target) { return false; };
};

