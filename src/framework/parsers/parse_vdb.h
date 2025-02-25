#pragma once

#include "parser.h"

class VdbParser : public Parser {
public:

    bool parse(std::string file_path, std::vector<Node*>& entities, uint32_t flags = PARSE_DEFAULT) override;
};
