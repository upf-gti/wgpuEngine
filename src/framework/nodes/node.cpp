#include "node.h"
#include "spdlog/spdlog.h"

void Node::render()
{
    for (Node* child : children) {
        child->render();
    }
}

void Node::update(float delta_time)
{
    for (Node* child : children) {
        child->update(delta_time);
    }
}

AABB Node::get_aabb() const
{
    AABB new_aabb = aabb;

    for (Node* child : children) {

        AABB child_aabb = child->get_aabb();

        if (!new_aabb.initialized()) {
            new_aabb = child_aabb;
            continue;
        }

        if (child_aabb.initialized()) {
            new_aabb = merge_aabbs(new_aabb, child_aabb);
        }
    }

    return new_aabb;
}
