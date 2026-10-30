#include "node_factory.h"

#include "node.h"

#include "assert.h"

NodeRegistration::NodeRegistration(const std::string& node_name, NodeFactory node_ctor)
{
    NodeRegistry::get_instance()->register_class(node_name, node_ctor);
}
