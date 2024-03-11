#include "node.h"

#include "framework/input.h"

#include "spdlog/spdlog.h"

std::map<std::string, std::vector<SignalType>> Node::mapping_signals;
std::map<uint8_t, std::vector<FuncEmpty>> Node::controller_signals;

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

void Node::remove_flag(uint8_t flag)
{
    for (Node* child : children) {
        child->remove_flag(flag);
    }
}

void Node::bind(const std::string& name, SignalType callback)
{
    mapping_signals[name].push_back(callback);
}

void Node::bind(uint8_t button, FuncEmpty callback)
{
    controller_signals[button].push_back(callback);
}

void Node::check_controller_signals()
{
    // Update controller buttons

    for (auto& it : controller_signals)
    {
        if (!Input::was_button_pressed(it.first))
            continue;

        for (auto& callback : it.second)
            callback();
    }
}
