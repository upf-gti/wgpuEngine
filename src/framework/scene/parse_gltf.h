#pragma once

#include <vector>
#include <string>

class Entity;

bool parse_gltf(const char* gltf_path, std::vector<Entity*>& entities);
