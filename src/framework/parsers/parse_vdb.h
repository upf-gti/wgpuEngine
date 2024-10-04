#pragma once

#include <vector>

class Node;

bool parse_vdb(const char* vdb_path, std::vector<Node*>& entities);
