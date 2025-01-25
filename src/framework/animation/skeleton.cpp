#include "skeleton.h"

#include <fstream>

Skeleton::Skeleton()
{
    ref();
}

Skeleton::Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names, const std::vector<uint32_t>& indices)
{
    set(rest, bind, names, indices);

    ref();
}

Skeleton::~Skeleton()
{

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

void Skeleton::serialize(std::ofstream& binary_scene_file)
{
    size_t name_size = name.size();
    binary_scene_file.write(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    binary_scene_file.write(name.c_str(), name_size);

    size_t joints_count = joint_ids.size();
    binary_scene_file.write(reinterpret_cast<char*>(&joints_count), sizeof(size_t));

    std::vector<int>& joint_parents = const_cast<std::vector<int>&>(bind_pose.get_parents());
    const std::vector<Transform>& joint_transforms_bind = bind_pose.get_joints();
    const std::vector<Transform>& joint_transforms_rest = rest_pose.get_joints();

    for (uint32_t i = 0u; i < joints_count; ++i) {
        // Name
        size_t joint_name_size = joint_names[i].size();
        binary_scene_file.write(reinterpret_cast<char*>(&joint_name_size), sizeof(size_t));
        binary_scene_file.write(joint_names[i].c_str(), joint_name_size);
        // ID
        size_t joint_id = joint_ids[i];
        binary_scene_file.write(reinterpret_cast<char*>(&joint_id), sizeof(size_t));
        // Parent
        int parent_id = joint_parents[i];
        binary_scene_file.write(reinterpret_cast<char*>(&parent_id), sizeof(int));
        // Transform (bind pose)
        Transform t = joint_transforms_bind[i];
        binary_scene_file.write(reinterpret_cast<char*>(&(t.get_position_ref())), sizeof(glm::vec3));
        binary_scene_file.write(reinterpret_cast<char*>(&t.get_rotation_ref()), sizeof(glm::quat));
        binary_scene_file.write(reinterpret_cast<char*>(&t.get_scale_ref()), sizeof(glm::vec3));
        // Transform (rest pose)
        t = joint_transforms_rest[i];
        binary_scene_file.write(reinterpret_cast<char*>(&(t.get_position_ref())), sizeof(glm::vec3));
        binary_scene_file.write(reinterpret_cast<char*>(&t.get_rotation_ref()), sizeof(glm::quat));
        binary_scene_file.write(reinterpret_cast<char*>(&t.get_scale_ref()), sizeof(glm::vec3));
    }
}

void Skeleton::parse(std::ifstream& binary_scene_file)
{
    size_t name_size = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    name.resize(name_size);
    binary_scene_file.read(&name[0], name_size);

    size_t joints_count = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&joints_count), sizeof(size_t));

    joint_ids.resize(joints_count);
    joint_names.resize(joints_count);
    bind_pose.resize(joints_count);
    rest_pose.resize(joints_count);

    for (uint32_t i = 0u; i < joints_count; ++i) {
        // Name
        size_t joint_name_size = 0;
        binary_scene_file.read(reinterpret_cast<char*>(&joint_name_size), sizeof(size_t));
        joint_names[i].resize(joint_name_size);
        binary_scene_file.read(&joint_names[i][0], joint_name_size);
        // ID
        size_t joint_id = 0;
        binary_scene_file.read(reinterpret_cast<char*>(&joint_id), sizeof(size_t));
        joint_ids[i] = joint_id;
        // Parent
        int parent_id = 0;
        binary_scene_file.read(reinterpret_cast<char*>(&parent_id), sizeof(int));
        bind_pose.set_parent(i, parent_id);
        rest_pose.set_parent(i, parent_id);
        // Transform (bind pose)
        Transform t;
        binary_scene_file.read(reinterpret_cast<char*>(&(t.get_position_ref())), sizeof(glm::vec3));
        binary_scene_file.read(reinterpret_cast<char*>(&t.get_rotation_ref()), sizeof(glm::quat));
        binary_scene_file.read(reinterpret_cast<char*>(&t.get_scale_ref()), sizeof(glm::vec3));
        bind_pose.set_local_transform(i, t);
        // Transform (rest pose)
        binary_scene_file.read(reinterpret_cast<char*>(&(t.get_position_ref())), sizeof(glm::vec3));
        binary_scene_file.read(reinterpret_cast<char*>(&t.get_rotation_ref()), sizeof(glm::quat));
        binary_scene_file.read(reinterpret_cast<char*>(&t.get_scale_ref()), sizeof(glm::vec3));
        rest_pose.set_local_transform(i, t);
    }

    current_pose = rest_pose;

    update_inv_bind_pose();
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

uint32_t Skeleton::get_joint_index(uint32_t id)
{
    return joint_ids[id];
}

uint32_t Skeleton::get_joints_count()
{
    return (uint32_t)joint_ids.size();
}

void Skeleton::update_inv_bind_pose()
{
    uint32_t size = (uint32_t)bind_pose.size();
    inv_bind_pose.resize(size);

    for (uint32_t i = 0; i < size; ++i) {
        Transform world = bind_pose.get_global_transform(i);
        inv_bind_pose[i] = glm::inverse(Transform::transform_to_mat4(world));
    }
}
