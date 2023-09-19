#include "json_utils.h"

#include <fstream>
#include <iostream>

json load_json(const std::string& filename) {

	json j;

    while (true) {

        std::ifstream ifs(filename.c_str());
        if (!ifs.is_open()) {
            std::cout << "Failed to open json file" << filename << std::endl;
            continue;
        }

#ifdef NDEBUG
        j = json::parse(ifs, nullptr, false);
        if (j.is_discarded()) {
            ifs.close();
			std::cout << "Failed to parse json file" << filename << std::endl;
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
			printf("Failed to parse json file %s\n%s\nAt offset: %zd"
                , filename.c_str(), e.what(), e.byte);
            continue;
        }
#endif
        // The json is correct, we can leave the while loop
        break;
    }

    return j;
}
