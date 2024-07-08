#include "scene.h"

#include "framework/nodes/node.h"
#include "scene_binary_format.h"

#include "engine/engine.h"

#include <fstream>

Scene::Scene(const std::string& name)
{
    this->name = name;
}

Scene::~Scene()
{
    delete_all();
}

void Scene::add_node(Node* node, int idx)
{
    if (idx == -1) {
        nodes.push_back(node);
    }
    else {
        // TODO: add as a child with global idx
    }

    Node::emit_signal(name + "@nodes_added", (void*)nullptr);
}

void Scene::add_nodes(const std::vector<Node*>& nodes_to_add, int idx)
{
    if (idx == -1) {
        nodes.insert(nodes.end(), nodes_to_add.begin(), nodes_to_add.end());
    }
    else {
        // TODO: add as a child with global idx
    }

    Node::emit_signal(name + "@nodes_added", (void*)nullptr);
}

// TODO: extend to allow removing nodes inside the hierachy
void Scene::remove_node(Node* node)
{
    // Checks if it's a child
    auto it = std::find(nodes.begin(), nodes.end(), node);
    if (it == nodes.end()) {
        return;
    }

    nodes.erase(it);
}

void Scene::set_name(const std::string& name)
{
    this->name = name;
}

std::vector<Node*>& Scene::get_nodes()
{
    return nodes;
}

void Scene::delete_all()
{
    for (auto node : nodes) {
        delete node;
    }

    nodes.clear();
}

void Scene::serialize(const std::string& path)
{
    std::ofstream binary_scene_file(path, std::ios::out | std::ios::binary);

    sSceneBinaryHeader header = {
        .version = 1,
        .node_count = nodes.size(),
    };

    binary_scene_file.write(reinterpret_cast<char*>(&header), sizeof(sSceneBinaryHeader));

    size_t name_size = name.size();
    binary_scene_file.write(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    binary_scene_file.write(name.c_str(), name_size);

    for (auto node : nodes) {
        node->serialize(binary_scene_file);
    }

    binary_scene_file.close();
}

void Scene::parse(const std::string& path)
{
    std::ifstream binary_scene_file(path, std::ios::in | std::ios::binary);

    sSceneBinaryHeader header;

    binary_scene_file.read(reinterpret_cast<char*>(&header), sizeof(sSceneBinaryHeader));

    // Update name if necessary
    size_t name_size = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    std::string new_name;
    new_name.resize(name_size);
    binary_scene_file.read(&new_name[0], name_size);

    if (name.empty()) {
        name = new_name;
    }

    Engine* engine = Engine::instance;

    std::string node_type;

    for (int i = 0; i < header.node_count; ++i) {
        // parse node type
        size_t node_type_size = 0;
        binary_scene_file.read(reinterpret_cast<char*>(&node_type_size), sizeof(size_t));
        node_type.resize(node_type_size);
        binary_scene_file.read(&node_type[0], node_type_size);

        Node* node = engine->node_factory(node_type);
        node->parse(binary_scene_file);
        add_node(node);
    }

    binary_scene_file.close();
}

void Scene::update(float delta_time)
{
    for (auto node : nodes) {
        node->update(delta_time);
    }
}

void Scene::render()
{
    for (auto node : nodes) {
        node->render();
    }
}
