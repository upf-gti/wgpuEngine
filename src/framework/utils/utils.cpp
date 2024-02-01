#include "utils.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "spdlog/spdlog.h"

#include "glm/gtx/hash.hpp"

std::string remove_special_characters(const std::string& str)
{
    std::string final_str = str;
    final_str.erase(std::remove_if(final_str.begin(), final_str.end(),
        [](char c) { return c == '\r' || c == '\n'; }),
        final_str.end());

    return final_str;
}

std::vector<std::string> tokenize(const std::string& str)
{
	std::vector<std::string> results;
    std::string::const_iterator start = str.begin();
    std::string::const_iterator end = str.end();
    std::string::const_iterator next = std::find(start, end, ' ');
    while (next != end) {
        results.push_back(std::string(start, next));
        start = next + 1;
        next = std::find(start, end, ' ');
    }

    results.push_back(remove_special_characters(std::string(start, next)));

    return results;
}

void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
    spdlog::error("{}(line {}) at {}\n{} - {}", p_function, p_line, p_file, p_error, p_message);
}

bool read_file(const std::string& filename, std::string& content)
{
	content.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
        spdlog::error("Error reading file ({}): {}", filename, strerror(errno));
		return false;
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	content.resize(size, ' ');

	file.seekg(0);
	file.read(content.data(), size);

    file.close();

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
}

uint32_t ceil_to_next_multiple(uint32_t value, uint32_t step)
{
    uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    return step * divide_and_ceil;
}

std::string delete_until_tags(std::istringstream& stream, std::string& text, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags)
{
    std::string current_tag;

    while (true) {
        std::getline(stream, current_line);
        auto tokens = tokenize(current_line);
        current_tag = tokens[0];

        auto it = std::find(tags.begin(), tags.end(), current_tag);

        if (it != tags.end()) {
            return *it;
        }
        else {
            text.replace(current_pos, current_line.length() + 1, "");
        }
    }
}

std::string continue_until_tags(std::istringstream& stream, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags)
{
    std::string current_tag;

    while (true) {
        std::getline(stream, current_line);
        auto tokens = tokenize(current_line);
        current_tag = tokens[0];

        auto it = std::find(tags.begin(), tags.end(), current_tag);

        if (it != tags.end()) {
            return *it;
        }
        else {
            current_pos += current_line.length() + 1;
        }
    }

    return "";
}

float clamp_rotation(float angle)
{
    constexpr float pi2 = 2.0f * glm::pi<float>();
    int turns = static_cast<int>(floor(angle / (pi2)));
    return angle - pi2 * turns;
}

glm::vec3 yaw_pitch_to_vector(float yaw, float pitch) {
    return glm::vec3(
        sinf(yaw) * cosf(-pitch),
        sinf(-pitch),
        cosf(yaw) * cosf(-pitch)
    );
}

void vector_to_yaw_pitch(const glm::vec3& front, float* yaw, float* pitch) {
    *yaw = atan2f(front.x, front.z);
    float mdo = sqrtf(front.x * front.x + front.z * front.z);
    *pitch = atan2f(-front.y, mdo);
}

// https://graemepottsfolio.wordpress.com/tag/damped-spring/
glm::vec3 smooth_damp(glm::vec3 current, glm::vec3 target, glm::vec3* current_velocity, float smooth_time, float max_speed, float delta_time)
{
    smooth_time = glm::max(0.0001f, smooth_time);
    float omega = 2.0f / smooth_time;
    float x = omega * delta_time;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    glm::vec3 deltaX = current - target;
    float maxDelta = max_speed * smooth_time;

    // ensure we do not exceed our max speed
    float length = glm::length(deltaX);
    if (length > maxDelta)
    {
        deltaX = glm::normalize(deltaX) * maxDelta;
    }
    else if (length < -maxDelta)
    {
        deltaX = glm::normalize(deltaX) * -maxDelta;
    }

    glm::vec3 temp = (static_cast<glm::vec3>(*current_velocity) + omega * deltaX) * delta_time;
    glm::vec3 result = static_cast<glm::vec3>(current) - deltaX + (deltaX + temp) * exp;
    *current_velocity = (static_cast<glm::vec3>(*current_velocity) - omega * temp) * exp;

    // ensure that we do not overshoot our target
    //if (glm::length(static_cast<glm::vec3>(target) - static_cast<glm::vec3>(current)) > 0.0f == glm::length(result) > glm::length(static_cast<glm::vec3>(target)))
    //{
    //    result = target;
    //    *current_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    //}

    return result;
}

// https://graemepottsfolio.wordpress.com/tag/damped-spring/
float smooth_damp(float current, float target, float* current_velocity, float smooth_time, float max_speed, float delta_time)
{
    smooth_time = std::max(0.0001f, smooth_time);
    float omega = 2.0f / smooth_time;
    float x = omega * delta_time;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float deltaX = current - target;
    float maxDelta = max_speed * smooth_time;

    // ensure we do not exceed our max speed
    deltaX = std::clamp(deltaX, -maxDelta, maxDelta);
    float temp = (*current_velocity + omega * deltaX) * delta_time;
    float result = current - deltaX + (deltaX + temp) * exp;
    *current_velocity = (*current_velocity - omega * temp) * exp;

    // ensure that we do not overshoot our target
    if (target - current > 0.0f == result > target)
    {
        result = target;
        *current_velocity = 0.0f;
    }
    return result;
}

float smooth_damp_angle(float current, float target, float* current_velocity, float smooth_time, float max_speed, float delta_time)
{
    float result;
    float diff = target - current;
    float pi2 = 2.0f * pi;
    if (diff < -pi)
    {
        // lerp upwards past PI_TIMES_TWO
        target += pi2;
        result = smooth_damp(current, target, current_velocity, smooth_time, max_speed, delta_time);
        if (result >= pi2)
        {
            result -= pi2;
        }
    }
    else if (diff > pi)
    {
        // lerp downwards past 0
        target -= pi2;
        result = smooth_damp(current, target, current_velocity, smooth_time, max_speed, delta_time);
        if (result < 0.f)
        {
            result += pi2;
        }
    }
    else
    {
        // straight lerp
        result = smooth_damp(current, target, current_velocity, smooth_time, max_speed, delta_time);
    }

    return result;
}
