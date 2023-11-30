#include "utils.h"

#include <regex>
#include <cassert>
#include <fstream>

#include "spdlog/spdlog.h"

std::vector<std::string> tokenize(const std::string& str) {
	
	std::regex reg("\\s+");

	std::sregex_token_iterator iter(str.begin(), str.end(), reg, -1);
	std::sregex_token_iterator end;

	std::vector<std::string> vec(iter, end);
	return vec;
}

void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
    spdlog::error("{}(line {}) at {}\n{} - {}", p_function, p_line, p_file, p_error, p_message);
}

bool read_file(const std::string& filename, std::string& content)
{
	content.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
        spdlog::error("File not found: {}", filename);
		return false;
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	content.resize(size, ' ');

	file.seekg(0);
	file.read(content.data(), size);

	return true;
}

glm::vec3 load_vec3(const std::string& str) {
    glm::vec3 v;
    int n = sscanf(str.c_str(), "%f %f %f", &v.x, &v.y, &v.z);
    if (n == 3) {
        return v;
    }
    printf("Invalid str reading VEC3 %s. Only %d values read. Expected 3\n", str.c_str(), n);
    return glm::vec3();
}

glm::vec4 load_vec4(const std::string& str) {
    glm::vec4 v;
    int n = sscanf(str.c_str(), "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
    if (n == 4) {
        return v;
    }
    printf("Invalid str reading VEC4 %s. Only %d values read. Expected 4\n", str.c_str(), n);
    return glm::vec4();
}

glm::quat load_quat(const std::string& str) {
    return glm::quat(load_vec4(str));
}

glm::vec3 mod_vec3(glm::vec3 v, float m)
{
    return glm::vec3(
        fmodf(v.x, m),
        fmodf(v.y, m),
        fmodf(v.z, m)
    );
}

// https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
uint32_t next_power_of_two(uint32_t value)
{
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}

glm::vec3 hsv2rgb(glm::vec3 c)
{
    glm::vec3 m = mod_vec3(c.x * 6.f + glm::vec3(0.f, 4.f, 2.f), 6.f);
    glm::vec3 rgb = glm::clamp(abs(m - 3.f) - 1.f, glm::vec3(0.f), glm::vec3(1.f));

    rgb = rgb * rgb * (3.f - 2.f * rgb); // cubic smoothing	

    return mix(glm::vec3(1.f), mix(glm::vec3(1.f), rgb, c.y), c.z);
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

glm::vec3 get_perpendicular(const glm::vec3& v)
{
    static float fZero = 1e-06f;

    glm::vec3 perp = glm::cross(v, glm::vec3(1.f, 0.f, 0.f));

    // Check length
    if (glm::length(perp) < fZero)
    {
        /* This vector is the Y axis multiplied by a scalar, so we have
        to use another axis.
            */
        perp = glm::cross(v, glm::vec3(0.f, 1.f, 0.f));
    }

    return perp;
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
