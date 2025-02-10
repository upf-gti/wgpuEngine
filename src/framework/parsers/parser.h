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

protected:
    std::future<bool> async_future;
    std::function<void(const std::vector<Node*>&, bool)> async_callback;
    std::vector<Node*> async_nodes;

    static std::vector<Parser*> async_parsers;

    virtual void on_async_finished() {};

public:
    virtual bool parse(const char* file_path, std::vector<Node*>& entities, uint32_t flags = PARSE_DEFAULT) { return false; };

    template<typename T>
    static void parse_async(const char* file_path, std::function<void(const std::vector<Node*>&, bool)> callback, uint32_t flags = PARSE_DEFAULT);

    static void poll_async_parsers();
};

template<typename T>
void Parser::parse_async(const char* file_path, std::function<void(const std::vector<Node*>&, bool)> callback, uint32_t flags)
{
    Parser* async_parser = new T();

    async_parser->async_callback = callback;

    // Start the async operation
    async_parser->async_future = std::async(std::launch::async, &Parser::parse, async_parser, file_path, std::ref(async_parser->async_nodes), flags);

    async_parsers.push_back(async_parser);
}
