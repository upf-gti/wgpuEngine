#pragma once

#include "framework/math.h"
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

constexpr float pi = glm::pi<float>();
constexpr float pi_2 = 0.5f * pi;

std::string remove_special_characters(const std::string& str);
std::vector<std::string> tokenize(const std::string & str);

void print_line(const char* line);
void print_line(const std::string & line);
void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message);

bool read_file(const std::string & filename, std::string & content);

void quat_swing_twist_decomposition(const glm::vec3& dir, const glm::quat& rotation, glm::quat& swing, glm::quat& twist);

glm::vec3 load_vec3(const std::string& str);
glm::vec4 load_vec4(const std::string& str);
glm::quat load_quat(const std::string& str);

glm::vec3 mod_vec3(glm::vec3 v, float m);
uint32_t next_power_of_two(uint32_t value);
glm::quat get_quat_between_vec3(const glm::vec3& p1, const glm::vec3& p2);

glm::vec3 hsv2rgb(glm::vec3 c);

float random_f(float range = 1.0f, int offset = 0);
glm::vec3 get_front(const glm::mat4 & pose);
glm::vec3 get_perpendicular(const glm::vec3& v);

uint32_t ceil_to_next_multiple(uint32_t value, uint32_t step);

#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                                                     \
	if (m_cond) {																														 \
		print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg);		 \
		return m_retval;                                                                                                                 \
	}

float bytes_to_float(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
unsigned int bytes_to_uint(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
unsigned short bytes_to_ushort(unsigned char b0, unsigned char b1);

// based on: https://stackoverflow.com/a/38140932
template <typename... Rest>
inline void hash_combine(std::size_t& seed, std::size_t v, const Rest... rest)
{
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

std::string delete_until_tags(std::istringstream& stream, std::string& text, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags);
std::string continue_until_tags(std::istringstream& stream, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags);

float clamp_rotation(float angle);

glm::vec3 yaw_pitch_to_vector(float yaw, float pitch);
void vector_to_yaw_pitch(const glm::vec3& front, float* yaw, float* pitch);

template<typename T>
struct LerpedValue {
    T value = {};
    T velocity = {};
};

glm::vec3 smooth_damp(glm::vec3 current, glm::vec3 target, glm::vec3* current_velocity, float smooth_time, float max_speed, float delta_time);
float smooth_damp(float current, float target, float* current_velocity, float smooth_time, float max_speed, float delta_time);
float smooth_damp_angle(float current, float target, float* current_velocity, float smooth_time, float max_speed, float delta_time);
