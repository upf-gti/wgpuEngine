#include "json_utils.h"

#include <fstream>

#include "core/managers/debug/debug_manager.h"

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

rapidjson::Document load_json(const std::string& filename) {

    rapidjson::Document j;

    while (true) {

        std::ifstream ifs(filename.c_str());
        if (!ifs.is_open()) {
            LOG_ERROR("Failed to open json file {}", filename);
            continue;
        }

        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document document;

        j.ParseStream(isw);
        if (j.HasParseError()) {
            ifs.close();
            LOG_ERROR("Failed to parse json file {}", filename);
            continue;
        }

        // The json is correct, we can leave the while loop
        break;
    }

    return j;
}
