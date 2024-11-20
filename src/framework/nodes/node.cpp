#include "node.h"

#include "framework/input.h"
#include "framework/utils/utils.h"
#include "framework/nodes/node_factory.h"

#include "node_factory.h"
#include "node_binary_format.h"

#include "engine/engine.h"

#include "spdlog/spdlog.h"
#include <fstream>

std::unordered_map<std::string, std::vector<SignalType>> Node::mapping_signals;
std::unordered_map<uint8_t, std::vector<FuncEmpty>> Node::controller_signals;
uint32_t Node::last_node_id = 0;

REGISTER_NODE_CLASS(Node)

Node::Node()
{
    node_type = "Node";

    name = "Node_" + std::to_string(last_node_id++);
}

void Node::add_child(Node* child)
{
    if (child->parent) {
        child->parent->remove_child(child);
    }

    // Checks if it's already a child
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        spdlog::error("Entity is already one of the children!");
        return;
    }

    child->parent = this;
    children.push_back(child);
}

void Node::remove_child(Node* child)
{
    // Checks if it's a child
    auto it = std::find(children.begin(), children.end(), child);
    if (it == children.end()) {
        spdlog::error("Entity is not a child!!");
        return;
    }

    children.erase(it);
    child->parent = nullptr;
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

void Node::serialize(std::ofstream& binary_scene_file)
{
    sNodeBinaryHeader header = {
        .children_count = children.size(),
    };

    size_t node_type_size = node_type.size();
    binary_scene_file.write(reinterpret_cast<char*>(&node_type_size), sizeof(size_t));
    binary_scene_file.write(node_type.c_str(), node_type_size);

    binary_scene_file.write(reinterpret_cast<char*>(&header), sizeof(sNodeBinaryHeader));

    size_t name_size = name.size();
    binary_scene_file.write(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    binary_scene_file.write(name.c_str(), name_size);

    for (auto child : children) {
        child->serialize(binary_scene_file);
    }
}

void Node::parse(std::ifstream& binary_scene_file)
{
    sNodeBinaryHeader header;

    binary_scene_file.read(reinterpret_cast<char*>(&header), sizeof(sNodeBinaryHeader));

    size_t name_size = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    name.resize(name_size);
    binary_scene_file.read(&name[0], name_size);

    std::string child_node_type;

    for (int i = 0; i < header.children_count; ++i) {

        // parse node type
        size_t node_type_size = 0;
        binary_scene_file.read(reinterpret_cast<char*>(&node_type_size), sizeof(size_t));
        child_node_type.resize(node_type_size);
        binary_scene_file.read(&child_node_type[0], node_type_size);

        Node* child = NodeRegistry::get_instance()->create_node(child_node_type);
        child->parse(binary_scene_file);
        add_child(child);
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

        Node* n = child->get_node(path_tokens);
        if (n) {
            return n->get_node(path_tokens);
        }
    }

    return nullptr;
}

Node* Node::get_node(const std::string& path)
{
    std::vector<std::string> path_tokens = tokenize(path, '/');
    return get_node(path_tokens);
}

Node::AnimatableProperty Node::get_animatable_property(const std::string& name)
{
    if (animatable_properties.contains(name)) {
        return animatable_properties[name];
    }

    return {};
}

const std::unordered_map<std::string, Node::AnimatableProperty>& Node::get_animatable_properties() const
{
    return animatable_properties;
}

std::string Node::find_path(const std::string& node_name, const std::string& current_path)
{
    if (get_name() == node_name) {
        return current_path;
    }

    for (Node* child : children) {
        std::string path = child->find_path(node_name, current_path + child->get_name() + "/");
        if (!path.empty()) {
            return path;
        }
    }

    return ""; // Node not found
}

void Node::clone(Node* new_node, bool copy)
{
    assert(new_node);
    new_node->set_name(get_name() + (copy ? "_copy" : "_instance"));
}

void Node::release()
{
    for (Node* child : children) {
        child->release();
    }
}

void Node::disable_2d()
{
    for (Node* child : children) {
        child->disable_2d();
    }
}

void Node::bind(const std::string& name, SignalType callback)
{
    mapping_signals[name].push_back(callback);
}

void Node::unbind(const std::string& name)
{
    if (mapping_signals.contains(name)) {
        mapping_signals.erase(name);
    }
}

void Node::bind_button(uint8_t button, FuncEmpty callback)
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

NodeRegistry* NodeRegistry::instance = nullptr;

NodeRegistry::NodeRegistry()
{
    assert(instance == nullptr);
    instance = this;
}

void NodeRegistry::register_class(const std::string& name, std::function<Node* ()> constructor)
{
    registry[name] = constructor;
}

Node* NodeRegistry::create_node(const std::string& name)
{
    std::string node_class = name;

    // legacy for rooms
    if (node_class == "SculptInstance") {
        node_class = "SculptNode";
    }

    Node* node = nullptr;

    if (registry.find(node_class) != registry.end()) {
        return registry[node_class]();
    }
    else {
        assert(0);
        return nullptr;
    }
}
