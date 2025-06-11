#pragma once
#include "framework/math/transform.h"
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
	Pose(size_t numJoints);

	// Resize the array of joints and of parents id
	void resize(size_t size);
    size_t size() const;

	void set_parent(size_t id, int parent_id);
    void set_parents(const std::vector<int>& new_parents) { parents = new_parents; };
    void set_joints(const std::vector<Transform>& new_joints) { joints = new_joints; };

	int get_parent(size_t id);
    const std::vector<Transform>& get_joints() const { return joints; };
    const std::vector<int>& get_parents() const { return parents; };

	// Set the transformation for the joint given its id
	void set_local_transform(size_t id, const Transform& transform);
	// Get the transformation of the joint given its id
	Transform& get_local_transform(size_t id);
	// Get the global transformation (world space) of the joint 
	Transform get_global_transform(size_t id);
	// Get the global transformation matrix (world space) of all the joints
	std::vector<glm::mat4> get_global_matrices();
	Transform operator[](size_t index);
    Pose& operator=(const Pose& p);
};
