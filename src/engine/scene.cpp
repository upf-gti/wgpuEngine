#include "scene.h"

#include "framework/nodes/node.h"

Scene::Scene()
{

}

Scene::~Scene()
{

}

void Scene::add_node(Node* node, int idx)
{
    if (idx == -1) {
        nodes.push_back(node);
    }
    else {
        // TODO: add as a child with global idx
    }
}

void Scene::add_nodes(const std::vector<Node*>& nodes_to_add, int idx)
{
    if (idx == -1) {
        nodes.insert(nodes.end(), nodes_to_add.begin(), nodes_to_add.end());
    }
    else {
        // TODO: add as a child with global idx
    }
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

void Scene::serialize_scene()
{

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
