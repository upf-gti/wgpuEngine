#pragma once

#include <glm/glm.hpp>
#include "json.hpp"
#include <regex>
#include <cassert>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

#define assert_msg(condition, msg) if (!(condition)) {std::cout << msg << std::endl; assert(false);}
#define _STR(m_x) #m_x

inline std::vector<std::string> tokenize(const std::string& str) {
	
	std::regex reg("\\s+");

	std::sregex_token_iterator iter(str.begin(), str.end(), reg, -1);
	std::sregex_token_iterator end;

	std::vector<std::string> vec(iter, end);
	return vec;
}

inline void print_line(const char* line) {
	std::cout << line << std::endl;
}

inline void print_line(const std::string & line) {
	std::cout << line << std::endl;
}

inline void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
	std::cout << p_function << "(line " << p_line << ") at " << p_file << std::endl << p_error << " - " << p_message << std::endl;
}

inline bool read_file(const std::string& filename, std::string& content)
{
	content.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "File not found " << filename << std::endl;
		return false;
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	content.resize(size, ' ');

	file.seekg(0);
	file.read(content.data(), size);

	return true;
}

inline json load_json(const std::string& filename) {

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

inline float random_f(float range = 1.0f, int offset = 0) {
	return ((rand() % 10000) / (10000.0f)) * range + offset;
}

inline glm::vec3 get_front(const glm::mat4& pose) {
	return -glm::normalize(pose[2]);
}

enum Error {
	OK,
	FAILED,
	ERR_NOT_FOUND,
	ERR_CANT_CREATE,
	ERR_MAX,
};

#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                                                     \
	if (m_cond) {																														 \
		print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg);		 \
		return m_retval;                                                                                                                 \
	}