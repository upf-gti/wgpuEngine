#pragma once

#include <vector>

class Node;
class Node3D;
class MeshInstance3D;

bool parse_scene(const char* scene_path, std::vector<Node*> &entities, bool fill_surface_data = false, Node3D* root = nullptr);
MeshInstance3D* parse_mesh(const char* mesh_path, bool create_aabb = true, bool fill_surface_data = false);
