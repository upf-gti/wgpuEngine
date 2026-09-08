#pragma once

#include <string>

class Node;

typedef Node* (*NodeFactory)(void);

class NodeRegistration
{
public:
    NodeRegistration(const std::string& node_name, NodeFactory node_ctor);
};

#define REGISTER_NODE_CLASS(node_ctor) \
    NodeRegistration _module_registration_ ## node_ctor(#node_ctor, []() -> Node* { return new node_ctor(); });
