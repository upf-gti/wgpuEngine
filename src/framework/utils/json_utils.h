#pragma once

#include "rapidjson/fwd.h"

#include <string>

rapidjson::Document load_json(const std::string& filename);
