#pragma once

#include "framework/entities/entity.h"

#include <vector>

class Entity;
class EntityMesh;

bool parse_scene(const char* scene_path, std::vector<Entity*> &entities);
EntityMesh* parse_mesh(const char* mesh_path);
