#include "utils.h"

#include <glm/glm.hpp>
#include "json.hpp"
#include <regex>
#include <cassert>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

std::vector<std::string> tokenize(const std::string& str) {
	
	std::regex reg("\\s+");

	std::sregex_token_iterator iter(str.begin(), str.end(), reg, -1);
	std::sregex_token_iterator end;

	std::vector<std::string> vec(iter, end);
	return vec;
}

void print_line(const char* line) {
	std::cout << line << std::endl;
}

void print_line(const std::string & line) {
	std::cout << line << std::endl;
}

void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
	std::cout << p_function << "(line " << p_line << ") at " << p_file << std::endl << p_error << " - " << p_message << std::endl;
}

bool read_file(const std::string& filename, std::string& content)
{
	content.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "\nFile not found " << filename << std::endl;
		return false;
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	content.resize(size, ' ');

	file.seekg(0);
	file.read(content.data(), size);

	return true;
}

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

glm::vec4 load_vec4(const std::string& str) {
    glm::vec4 v;
    int n = sscanf(str.c_str(), "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
    if (n == 4) {
        return v;
    }
    printf("Invalid str reading VEC4 %s. Only %d values read. Expected 4", str.c_str(), n);

    return glm::vec4();
}

glm::vec4 load_vec4(const json& j, const char* attr, const glm::vec4& defaultValue) {

    assert(j.is_object());
    if (j.count(attr)) {
        const std::string& str = j.value(attr, "");
        return load_vec4(str);
    }

    return defaultValue;
}

glm::vec3 rotate_point_by_quat(const glm::vec3& v, const glm::vec4& q) {
	const glm::vec3 q_vect = glm::vec3(q.x, q.y, q.z);
	return v + 2.0f * glm::cross(q_vect, glm::cross(q_vect, v) + q.w * v);
}

float random_f(float range, int offset) {
	return ((rand() % 10000) / (10000.0f)) * range + offset;
}

glm::vec3 get_front(const glm::mat4& pose) {
	return -glm::normalize(pose[2]);
}

float bytes_to_float(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3)
{
    float output;

    *((unsigned char*)(&output) + 3) = b3;
    *((unsigned char*)(&output) + 2) = b2;
    *((unsigned char*)(&output) + 1) = b1;
    *((unsigned char*)(&output) + 0) = b0;

    return output;
};

unsigned int bytes_to_uint(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3)
{
    unsigned int output;

    *((unsigned char*)(&output) + 3) = b3;
    *((unsigned char*)(&output) + 2) = b2;
    *((unsigned char*)(&output) + 1) = b1;
    *((unsigned char*)(&output) + 0) = b0;

    return output;
};

unsigned short bytes_to_ushort(unsigned char b0, unsigned char b1)
{
    unsigned short output;

    *((unsigned char*)(&output) + 1) = b1;
    *((unsigned char*)(&output) + 0) = b0;

    return output;
};
