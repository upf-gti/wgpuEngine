#pragma once

#include "framework/nodes/node.h"

#include <map>
#include <string>
#include <vector>

#include <future>

enum eParseFlags {
     PARSE_NO_FLAGS = 0,
     PARSE_GLTF_CLEAR_CACHE = 1 << 0,
     PARSE_GLTF_FILL_SURFACE_DATA = 1 << 1,
     PARSE_DEFAULT = PARSE_GLTF_CLEAR_CACHE
};

class Parser {

    std::future<bool> async_future;
    std::function<void()> async_callback;
    std::vector<Node*> async_entities;

public:
    virtual bool parse(const char* file_path, std::vector<Node*>& entities, uint32_t flags = PARSE_DEFAULT) { return false; };
    void parse_async(const char* file_path, std::function<void()> callback, uint32_t flags = PARSE_DEFAULT);

    bool poll_async();

    std::vector<Node*> get_async_entities() { return async_entities; }
};
