#pragma once

class MeshInstance3D;

void parse_obj(const char* obj_path, MeshInstance3D* entity);
MeshInstance3D* parse_obj(const char* obj_path);
