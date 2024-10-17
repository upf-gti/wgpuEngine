#include "default_node_factory.h"

#include "node_2d.h"
#include "mesh_instance_3d.h"
#include "skeleton_instance_3d.h"
#include "environment_3d.h"
#include "omni_light_3d.h"
#include "directional_light_3d.h"
#include "spot_light_3d.h"

#include "assert.h"

NodeFactory* NodeFactory::instance = nullptr;

NodeFactory::NodeFactory()
{
    assert(instance == nullptr);
    instance = this;

    REGISTER_NODE_TYPE(Node)
    REGISTER_NODE_TYPE(Node2D)
    REGISTER_NODE_TYPE(Node3D)
    REGISTER_NODE_TYPE(MeshInstance3D)
    REGISTER_NODE_TYPE(Environment3D)
    REGISTER_NODE_TYPE(OmniLight3D)
    REGISTER_NODE_TYPE(DirectionalLight3D)
    REGISTER_NODE_TYPE(SpotLight3D)
}

Node* default_node_factory(const std::string& node_type)
{
    std::string node_class = node_type;

    // legacy
    if (node_class == "SculptInstance") {
        node_class = "SculptNode";
    }

    Node* node = NodeFactory::get_instance()->create_node(node_class);

    if (!node) {
        assert(0);
    }

    return node;
}
