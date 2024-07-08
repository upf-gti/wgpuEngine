#include "default_node_factory.h"

#include "node_2d.h"
#include "mesh_instance_3d.h"
#include "environment_3d.h"

Node* default_node_factory(const std::string& node_type)
{
    Node* node = nullptr;

    if (node_type == "Node") {
        node = new Node();
    } else
    if (node_type == "Node2D") {
        node = new Node2D();
    } else
    if (node_type == "Node3D") {
        node = new Node3D();
    } else
    if (node_type == "MeshInstance3D") {
        node = new MeshInstance3D();
    } else
    if (node_type == "Environment3D") {
        node = new Environment3D();
    }
    else {
        assert(0);
    }

    return node;
}
