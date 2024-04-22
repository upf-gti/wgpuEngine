#include "skeleton.h"

Skeleton::Skeleton()
{

}

Skeleton::Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names, const std::vector<uint32_t>& indices)
{
    set(rest, bind, names, indices);
}

void Skeleton::set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names, const std::vector<uint32_t>& indices)
{
    rest_pose = rest;
    bind_pose = bind;
    current_pose = rest;
    joint_names = names;
    if (joint_names.size() != indices.size()) {
        joint_ids.resize(joint_names.size());
        for (uint32_t i = 0; i < joint_names.size(); i++) {
            joint_ids[i] = i;
        }
    }
    else {
        joint_ids = indices;
    }

    update_inv_bind_pose();
}

Skeleton::~Skeleton()
{

}

Pose& Skeleton::get_bind_pose()
{
    return bind_pose;
}

Pose& Skeleton::get_rest_pose()
{
    return rest_pose;
}

Pose& Skeleton::get_current_pose()
{
    return current_pose;
}

void Skeleton::set_current_pose(const Pose& pose)
{
    current_pose.set_joints(pose.get_joints());
}

const std::vector<glm::mat4>& Skeleton::get_inv_bind_pose()
{
    return inv_bind_pose;
}

const std::vector<std::string>& Skeleton::get_joint_names()
{
    return joint_names;
}

std::string& Skeleton::get_joint_name(uint32_t id)
{
    return joint_names[id];
}

const std::vector<uint32_t>& Skeleton::get_joint_indices()
{
    return joint_ids;
}

uint32_t& Skeleton::get_joint_indices(uint32_t id)
{
    return joint_ids[id];
}

uint32_t Skeleton::get_joints_count()
{
    return (uint32_t)joint_ids.size();
}

void Skeleton::update_inv_bind_pose()
{
    uint32_t size = bind_pose.size();
    inv_bind_pose.resize(size);

    for (uint32_t i = 0; i < size; ++i) {
        Transform world = bind_pose.get_global_transform(i);
        inv_bind_pose[i] = inverse(transformToMat4(world));
    }
}
