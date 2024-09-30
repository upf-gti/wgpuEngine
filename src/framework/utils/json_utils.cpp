#include "json_utils.h"

#include <fstream>

#include "spdlog/spdlog.h"

#include "json.hpp"

json load_json(const std::string& filename) {

	json j;

    while (true) {

        std::ifstream ifs(filename.c_str());
        if (!ifs.is_open()) {
            spdlog::error("Failed to open json file {}", filename);
            continue;
        }

        j = json::parse(ifs, nullptr, false);
        if (j.is_discarded()) {
            ifs.close();
            spdlog::error("Failed to parse json file {}", filename);
            continue;
        }

        // The json is correct, we can leave the while loop
        break;
    }

    return j;
}
