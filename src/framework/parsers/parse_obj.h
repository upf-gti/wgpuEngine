#pragma once

#include <string>

class MeshInstance3D;

void parse_obj(const std::string& obj_path, MeshInstance3D* entity_mesh, bool create_aabb = true);
MeshInstance3D* parse_obj(const std::string& obj_path, bool create_aabb = true);
