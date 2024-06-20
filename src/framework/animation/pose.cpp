#include "pose.h"

Pose::Pose()
{

}

Pose::Pose(size_t numJoints)
{
    resize(numJoints);
}

Pose::Pose(const Pose& p)
{
    *this = p;
}

// resize arrays
void Pose::resize(size_t size)
{
    parents.resize(size);
    joints.resize(size);
}

// get the number of joints
size_t Pose::size() const
{
    return joints.size();
}

// set parent id
void Pose::set_parent(size_t id, uint32_t parent_id)
{
    parents[id] = parent_id;
}

// get parent id
int Pose::get_parent(size_t id)
{
    return parents[id];
}

//set local transform of the joint
void Pose::set_local_transform(size_t id, const Transform& transform)
{
    joints[id] = transform;
}

// get local transform of the joint
Transform& Pose::get_local_transform(size_t id)
{
    return joints[id];
}

// get global (world) transform of the joint
Transform Pose::get_global_transform(size_t id)
{
    if (id >= joints.size()) {
        assert(0);
    }

    Transform transform = joints[id];
    for (int i = parents[id]; i >= 0; i = parents[i]) {
        if (i >= joints.size()) {
            break;
        }
        transform = Transform::combine(joints[i], transform);
    }
    return transform;
}

Transform Pose::operator[](size_t index)
{
    return get_global_transform(index);
}

Pose& Pose::operator=(const Pose& other) {
    if (this == &other) {
        return *this;
    }
    joints = other.joints;
    parents = other.parents;
    return *this;
}

// get global matrices of the joints
std::vector<glm::mat4> Pose::get_global_matrices()
{
    uint32_t numJoints = (uint32_t)size();
    std::vector<glm::mat4> out(numJoints);
    if (out.size() != numJoints) {
        out.resize(numJoints);
    }

    for (uint32_t i = 0; i < numJoints; i++) {
        Transform t = get_global_transform(i);
        out[i] = Transform::transform_to_mat4(t);
    }

    return out;
}
