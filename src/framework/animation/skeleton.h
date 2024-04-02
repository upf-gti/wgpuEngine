#pragma once

#include "pose.h"
#include <string>

class Skeleton
{
protected:
	Pose bind_pose;
	Pose rest_pose;

	std::vector<glm::mat4> inv_bind_pose; // vector of inverse bind pose matrix of each joint
	std::vector<std::string> joint_names; // vector of the name of each joint

	// updates the inverse bind pose matrices: any time the bind pose of the skeleton is updated, the inverse bind pose should be re-calculated as well
	void update_inv_bind_pose();
public:

	Skeleton(); // Empty constructor
	// Initialize the skeleton given the rest and bind poses, and the names of the joints
	Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names);

	void set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names);

	Pose& get_bind_pose();
	Pose& get_rest_pose();

	std::vector<glm::mat4>& get_inv_bind_pose();
	std::vector<std::string>& get_joint_names();
	std::string& get_joint_name(unsigned int id);
};
