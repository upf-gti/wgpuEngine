#pragma once
#include "framework/animation/bone_transform.h"
#include <vector>

// Used to hold the transformation of every bone in an animated hierarchy
class Pose
{
protected:
	std::vector<Transform> joints; // local transforms
	std::vector<int> parents; // parent joints Id (index in the joints array)
public:

	Pose(); // Empty constructor
	// Initialize the pose given another pose
	Pose(const Pose& p);
	// Initialize the pose given the number of joints of the pose
	Pose(uint32_t numJoints);

	// Resize the array of joints and of parents id
	void resize(uint32_t size);
	uint32_t size() const;

	void set_parent(uint32_t id, uint32_t parent_id);
    void set_joints(std::vector<Transform> new_joints) { joints = new_joints; };

	int get_parent(uint32_t id);
    const std::vector<Transform>& get_joints() const { return joints; };

	// Set the transformation for the joint given its id
	void set_local_transform(uint32_t id, const Transform& transform);
	// Get the transformation of the joint given its id
	Transform& get_local_transform(uint32_t id);
	// Get the global transformation (world space) of the joint 
	Transform get_global_transform(uint32_t id);
	// Get the global transformation matrix (world space) of all the joints
	std::vector<glm::mat4> get_global_matrices();
	Transform operator[](uint32_t index);
};
