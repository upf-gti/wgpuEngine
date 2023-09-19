#pragma once

#include "json.hpp"

using json = nlohmann::json;

json load_json(const std::string& filename);
