#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>

enum Error {
    OK,
    FAILED,
    ERR_NOT_FOUND,
    ERR_CANT_CREATE,
    ERR_MAX,
};

#define assert_msg(condition, msg) if (!(condition)) { spdlog::error(msg); assert(false);}
#define _STR(m_x) #m_x

std::vector<std::string> tokenize(const std::string & str);

void print_line(const char* line);
void print_line(const std::string & line);
void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message);

bool read_file(const std::string & filename, std::string & content);

glm::vec3 load_vec3(const std::string& str);
glm::vec4 load_vec4(const std::string& str);
glm::quat load_quat(const std::string& str);

glm::vec3 mod_vec3(glm::vec3 v, float m);
uint32_t next_power_of_two(uint32_t value);

glm::vec3 hsv2rgb(glm::vec3 c);

float random_f(float range = 1.0f, int offset = 0);
glm::vec3 get_front(const glm::mat4 & pose);
glm::vec3 get_perpendicular(const glm::vec3& v);

#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                                                     \
	if (m_cond) {																														 \
		print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg);		 \
		return m_retval;                                                                                                                 \
	}

float bytes_to_float(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
unsigned int bytes_to_uint(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
unsigned short bytes_to_ushort(unsigned char b0, unsigned char b1);
