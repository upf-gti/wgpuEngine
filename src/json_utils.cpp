#include "json_utils.h"

#include <fstream>

#include "spdlog/spdlog.h"

json load_json(const std::string& filename) {

	json j;

    while (true) {

        std::ifstream ifs(filename.c_str());
        if (!ifs.is_open()) {
            spdlog::error("Failed to open json file {}", filename);
            continue;
        }

#ifdef NDEBUG
        j = json::parse(ifs, nullptr, false);
        if (j.is_discarded()) {
            ifs.close();
            spdlog::error("Failed to parse json file {}", filename);
            continue;
        }
#else
        try
        {
            j = json::parse(ifs);
        }
        catch (json::parse_error& e)
        {
            ifs.close();
            // Output exception information
            spdlog::error("Failed to parse json file {}\n{}\nAt offset: {}", filename.c_str(), e.what(), e.byte);
            continue;
        }
#endif
        // The json is correct, we can leave the while loop
        break;
    }

    return j;
}
