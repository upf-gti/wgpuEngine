#pragma once

#include "framework/entities/entity.h"

class EntityMesh;

bool parse_scene(const std::string& scene_path, std::vector<Entity*> &entities);
EntityMesh* parse_mesh(const std::string& mesh_path);
