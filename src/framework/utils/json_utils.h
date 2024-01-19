#pragma once

#include "json_fwd.hpp"

using json = nlohmann::json;

json load_json(const std::string& filename);
