#pragma once

#include <vector>

class Node;

bool parse_ply(const char* ply_path, std::vector<Node*>& entities);
