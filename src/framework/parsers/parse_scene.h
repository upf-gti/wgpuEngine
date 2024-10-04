#pragma once

#include <vector>

class Node;
class MeshInstance3D;

bool parse_scene(const char* scene_path, std::vector<Node*> &entities);
MeshInstance3D* parse_mesh(const char* mesh_path, bool create_aabb = true);
