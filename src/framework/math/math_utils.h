#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#include <string>

#define PI 3.14159265358979323846264338327950288
#define PI_2 (0.5f * PI)
#define PI_4 (0.5f * PI_2)

const double infinity = std::numeric_limits<double>::infinity();

void quat_swing_twist_decomposition(const glm::vec3& dir, const glm::quat& rotation, glm::quat& swing, glm::quat& twist);

glm::vec3 load_vec3(const std::string& str);
glm::vec4 load_vec4(const std::string& str);
glm::quat load_quat(const std::string& str);

uint32_t log2(uint32_t value);

glm::vec3 mod_vec3(glm::vec3 v, float m);
uint32_t next_power_of_two(uint32_t value);
glm::quat get_quat_between_vec3(const glm::vec3& p1, const glm::vec3& p2);

glm::vec3 rgb2hsv(glm::vec3 rgb);
glm::vec3 hsv2rgb(glm::vec3 c);

float random_f(float min = 0.0f, float max = 1.0f);
double random_d(double min = 0.0, double max = 1.0);

glm::dvec3 random_direction();
glm::dvec3 random_direction(double min, double max);

glm::dvec3 random_unit_sphere_direction();
glm::dvec3 random_unit_hemisphere_direction(const glm::dvec3& normal);

bool is_direction_near_zero(const glm::dvec3& direction);

glm::vec3 get_front(const glm::mat4& pose);
glm::vec3 get_perpendicular(const glm::vec3& v);

uint32_t ceil_to_next_multiple(uint32_t value, uint32_t step);

float bytes_to_float(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
unsigned int bytes_to_uint(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
unsigned short bytes_to_ushort(unsigned char b0, unsigned char b1);

float clamp_rotation(float angle);

float remap_range(float old_value, float old_min, float old_max, float new_min, float new_max);

glm::vec3 yaw_pitch_to_vector(float yaw, float pitch);
void vector_to_yaw_pitch(const glm::vec3& front, float* yaw, float* pitch);

glm::quat get_rotation_to_face(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);

template<typename T>
struct LerpedValue {
    T value = {};
    T velocity = {};
};

glm::vec3 smooth_damp(glm::vec3 current, glm::vec3 target, glm::vec3* current_velocity, float smooth_time, float max_speed, float delta_time);
float smooth_damp(float current, float target, float* current_velocity, float smooth_time, float max_speed, float delta_time);
float smooth_damp_angle(float current, float target, float* current_velocity, float smooth_time, float max_speed, float delta_time);
