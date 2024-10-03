#pragma once

class MeshInstance3D;

void parse_obj(const char* obj_path, MeshInstance3D* entity_mesh, bool create_aabb = true);
MeshInstance3D* parse_obj(const char* obj_path, bool create_aabb = true);
