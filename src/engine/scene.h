#pragma once

#include <vector>

class Node;

class Scene {

public:
    Scene();
    ~Scene();

    void add_node(Node* node, int idx = -1);
    void add_nodes(const std::vector<Node*>& nodes_to_add, int idx = -1);

    std::vector<Node*>& get_nodes();

    void delete_all();

    void serialize_scene();

    void update(float delta_time);
    void render();

private:

    std::vector<Node*> nodes;

};
