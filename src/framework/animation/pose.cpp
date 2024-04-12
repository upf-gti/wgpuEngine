#include "pose.h"

Pose::Pose() { }

Pose::Pose(unsigned int numJoints) {
	resize(numJoints);
}

Pose::Pose(const Pose& p) {
	*this = p;
}

// resize arrays
void Pose::resize(unsigned int size) {
	parents.resize(size);
	joints.resize(size);
}

// get the number of joints
unsigned int Pose::size() {
	return joints.size();
}

// set parent id
void Pose::set_parent(unsigned int id, unsigned int parent_id) {
	parents[id] = parent_id;
}

// get parent id
int Pose::get_parent(unsigned int id) {
	return parents[id];
}

//set local transform of the joint
void Pose::set_local_transform(unsigned int id, const Transform& transform) {
	joints[id] = transform;
}

// get local transform of the joint
Transform& Pose::get_local_transform(unsigned int id) {
	return joints[id];
}

// get global (world) transform of the joint
Transform& Pose::get_global_transform(unsigned int id) {

    if (id >= joints.size()) {
        assert(0);
    }

	Transform transform = joints[id];
	for (int i = parents[id]; i >= 0; i = parents[i]) {
        if (i >= joints.size()) {
            break;
        }
		transform = combine(joints[i], transform);
	}
	return transform;
}

Transform Pose::operator[](unsigned int index) {
	return get_global_transform(index);
}

// get global matrices of the joints
std::vector<glm::mat4> Pose::get_global_matrices() {
	unsigned int numJoints = size();
	std::vector<glm::mat4> out(numJoints);
	if (out.size() != numJoints) {
		out.resize(numJoints);
	}


	for (unsigned int i = 0; i < numJoints; i++) {
		Transform t = get_global_transform(i);
		out[i] = transformToMat4(t);
	}

	return out;
}
