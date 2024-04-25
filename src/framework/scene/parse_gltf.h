#pragma once

#include <vector>

class Node3D;

bool parse_gltf(const char* gltf_path, std::vector<Node3D*>& entities);
