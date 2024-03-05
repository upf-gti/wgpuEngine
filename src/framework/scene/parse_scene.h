#pragma once

#include <vector>

class Node3D;
class MeshInstance3D;

bool parse_scene(const char* scene_path, std::vector<Node3D*> &entities);
MeshInstance3D* parse_mesh(const char* mesh_path);
