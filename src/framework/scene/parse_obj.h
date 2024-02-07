#pragma once

class EntityMesh;

void parse_obj(const char* obj_path, EntityMesh* entity);
EntityMesh* parse_obj(const char* obj_path);
