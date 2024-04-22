#pragma once

#include <string>

#include "pose.h"

class Skeleton
{
    std::string name = "";

    Pose bind_pose;
    Pose rest_pose;
    Pose current_pose;

    std::vector<glm::mat4> inv_bind_pose; // vector of inverse bind pose matrix of each joint
    std::vector<std::string> joint_names; // vector of the name of each joint
    std::vector<uint32_t> joint_ids; // vector of the id of each joint

    // updates the inverse bind pose matrices: any time the bind pose of the skeleton is updated, the inverse bind pose should be re-calculated as well
    void update_inv_bind_pose();

public:

    Skeleton(); // Empty constructor

    // Initialize the skeleton given the rest and bind poses, and the names of the joints
    Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names, const std::vector<uint32_t>& ids = {});

    virtual ~Skeleton();

    void set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names, const std::vector<uint32_t>& ids = {});

    Pose& get_bind_pose();
    Pose& get_rest_pose();
    Pose& get_current_pose();

    const std::vector<glm::mat4>& get_inv_bind_pose();
    const std::vector<std::string>& get_joint_names();
    const std::vector<uint32_t>& get_joint_indices();
    std::string& get_joint_name(uint32_t id);
    uint32_t& get_joint_indices(uint32_t id);
    uint32_t get_joints_count();
    const std::string& get_name() { return name; };

    void set_current_pose(const Pose& pose);
    void set_name(const std::string& new_name) { name = new_name; };
};
