#include "json_utils.h"

#include <fstream>

#include "spdlog/spdlog.h"

#include "rapidjson/document.h"

rapidjson::Document load_json(const std::string& filename) {

    rapidjson::Document j;

    while (true) {

        std::ifstream ifs(filename.c_str());
        if (!ifs.is_open()) {
            spdlog::error("Failed to open json file {}", filename);
            continue;
        }

        j.Parse(filename.c_str());
        if (j.HasParseError()) {
            ifs.close();
            spdlog::error("Failed to parse json file {}", filename);
            continue;
        }

        // The json is correct, we can leave the while loop
        break;
    }

    return j;
}
