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
	Pose(unsigned int numJoints);

	// Resize the array of joints and of parents id
	void resize(unsigned int size);
	unsigned int size();

	void set_parent(unsigned int id, unsigned int parent_id);
	int get_parent(unsigned int id);

	// Set the transformation for the joint given its id
	void set_local_transform(unsigned int id, const Transform& transform);
	// Get the transformation of the joint given its id
	Transform get_local_transform(unsigned int id);
	// Get the global transformation (world space) of the joint 
	Transform get_global_transform(unsigned int id);
	// Get the global transformation matrix (world space) of all the joints
	std::vector<glm::mat4> get_global_matrices();
	Transform operator[](unsigned int index);
};
