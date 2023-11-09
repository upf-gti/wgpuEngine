#pragma once

#include "framework/entities/entity_mesh.h"

#include <vector>

void parse_gltf(const std::string& gltf_path, std::vector<Entity*>& entities);
