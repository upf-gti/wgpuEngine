#pragma once

#include <map>
#include <string>
#include <vector>

class Node;

enum eParseFlags {
     PARSE_NO_FLAGS = 0,
     PARSE_GLTF_CLEAR_CACHE = 1 << 0,
     PARSE_GLTF_FILL_SURFACE_DATA = 1 << 1,
     PARSE_DEFAULT = PARSE_GLTF_CLEAR_CACHE
};

class Parser {
public:
    virtual bool parse(const char* file_path, std::vector<Node*>& entities, uint32_t flags = PARSE_DEFAULT) { return false; };
};
