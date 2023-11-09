#pragma once

#include "framework/entities/entity_text.h"

void parse_scene(const std::string& scene_path, std::vector<Entity*> &entities);
EntityMesh* parse_mesh(const std::string& mesh_path);
