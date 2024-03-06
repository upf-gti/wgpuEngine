#pragma once

#include "framework/math.h"
#include "framework/aabb.h"

#include <string>
#include <vector>

enum NodeType {
    NODE_2D,
    NODE_3D
};

class Node {

protected:

	std::string name;

    NodeType type;

	std::vector<Node*> children;

    AABB aabb = {};

public:

    Node() {};
	virtual ~Node() {};

	virtual void render();
	virtual void update(float delta_time);

    std::string get_name() const { return name; }
    std::vector<Node*>& get_children() { return children; }
    AABB get_aabb() const;

    void set_name(std::string name) { this->name = name; }
    void set_aabb(const AABB& aabb) { this->aabb = aabb; }
};
