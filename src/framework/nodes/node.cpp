#include "node.h"

#include "framework/input.h"
#include "framework/utils/utils.h"

#include "spdlog/spdlog.h"

std::map<std::string, std::vector<SignalType>> Node::mapping_signals;
std::map<uint8_t, std::vector<FuncEmpty>> Node::controller_signals;
uint32_t Node::last_node_id = 0;

Node::Node()
{
    name = "Node_" + std::to_string(last_node_id++);
}

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

Node* Node::get_node(std::vector<std::string>& path_tokens)
{
    if (!path_tokens.size() || path_tokens[0] == "") {
        return this;
    }

    if (this->name == path_tokens[0]) {
        path_tokens.erase(path_tokens.begin());
        return this->get_node(path_tokens);
    }

    for (Node* child : children) {

        if (child->get_name() == path_tokens[0]) {
            path_tokens.erase(path_tokens.begin());
            return child->get_node(path_tokens);
        }
    }

    return nullptr;
}

Node* Node::get_node(const std::string& path)
{
    std::vector<std::string> path_tokens = tokenize(path, '/');
    return get_node(path_tokens);
}

void* Node::get_property(const std::string& name)
{
    if (properties.contains(name)) {
        return properties[name];
    }

    return nullptr;
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
