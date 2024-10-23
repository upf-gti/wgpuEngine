#pragma once

#include <vector>

class Node;

bool parse_gltf(const char* gltf_path, std::vector<Node*>& entities, bool fill_surface_data = false);
