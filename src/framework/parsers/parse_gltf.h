#pragma once

#include <vector>

class Node;
class Node3D;

bool parse_gltf(const char* gltf_path, std::vector<Node*>& entities, bool fill_surface_data = false, Node3D* root = nullptr);
