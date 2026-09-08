#pragma once

#include <string>
#include <vector>

class Node;
class Camera3D;

class Scene {
public:
    Scene();
    Scene(const std::string& name);
    ~Scene();

    void add_node(Node* node, int idx = -1);
    void add_nodes(const std::vector<Node*>& nodes_to_add, int idx = -1);

    void remove_node(Node* node);

    void set_name(const std::string& name);

    std::vector<Node*>& get_nodes();
    const std::string& get_name() const { return name; }

    void set_main_camera(Camera3D* camera);
    Camera3D* get_main_camera();

    void delete_all();

    void serialize(const std::string& path);
    void parse(const std::string& path);

    void update(float delta_time);
    void render();

private:
    std::vector<Node*> nodes;

    std::string name;

    Camera3D* main_camera = nullptr;
};
