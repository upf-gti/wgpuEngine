#include "skeleton.h"

Skeleton::Skeleton() {}

Skeleton::Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names) {
	set(rest, bind, names);
}

void Skeleton::set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names) {
	rest_pose = rest;
	bind_pose = bind;
    current_pose = rest;
	joint_names = names;
	update_inv_bind_pose();
}

Pose& Skeleton::get_bind_pose() {
	return bind_pose;
}

Pose& Skeleton::get_rest_pose() {
	return rest_pose;
}

Pose& Skeleton::get_current_pose() {
    return current_pose;
}

void Skeleton::set_current_pose(Pose pose) {
    current_pose = pose;
}

std::vector<glm::mat4>& Skeleton::get_inv_bind_pose() {
	return inv_bind_pose;
}

std::vector<std::string>& Skeleton::get_joint_names() {
	return joint_names;
}

std::string& Skeleton::get_joint_name(unsigned int id) {
	return joint_names[id];
}

void Skeleton::update_inv_bind_pose() {
	unsigned int size = bind_pose.size();
	inv_bind_pose.resize(size);
	for (unsigned int i = 0; i < size; ++i) {
		Transform world = bind_pose.get_global_transform(i);
		inv_bind_pose[i] = inverse(transformToMat4(world));
	}
}
