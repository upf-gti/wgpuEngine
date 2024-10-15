#include "math_utils.h"

#include <glm/gtx/projection.hpp>

#include <algorithm>
#include <bit>

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

uint32_t log2(uint32_t value)
{
    return std::bit_width(value) - 1;
}

glm::vec3 mod_vec3(glm::vec3 v, float m)
{
    return glm::vec3(
        fmodf(v.x, m),
        fmodf(v.y, m),
        fmodf(v.z, m)
    );
}
glm::quat get_quat_between_vec3(const glm::vec3& p1, const glm::vec3& p2) {
    const float facing = glm::dot(p1, p2);
    if (facing > 0.9999f || facing < -0.9999f) {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }
    const glm::vec3 closs_p = glm::cross(p1, p2);
    const float a = sqrtf(powf(glm::length(p1), 2.0f) * powf(glm::length(p1), 2.0f)) + facing;
    return glm::normalize(glm::quat{ closs_p.x, closs_p.y, closs_p.z, a });
}


// Swing Twist decomposition for quaternions
// https://stackoverflow.com/questions/3684269/component-of-a-quaternion-rotation-around-an-axis
void quat_swing_twist_decomposition(const glm::vec3& dir, const glm::quat& rotation, glm::quat& swing, glm::quat& twist) {
    const glm::vec3 rotation_axis = glm::vec3(rotation.x, rotation.y, rotation.z);
    const glm::vec3 p = glm::proj(rotation_axis, dir);

    twist = glm::normalize(glm::quat(p.x, p.y, p.z, rotation.w));
    swing = rotation * glm::conjugate(twist);
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
    c.x /= 360.f;
    glm::vec4 K = { 1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0 };
    glm::vec3 p = glm::abs(glm::fract(glm::vec3(c.x) + glm::vec3(K.x, K.y, K.z)) * 6.0f - glm::vec3(K.w));
    return c.z * mix(glm::vec3(K.x), clamp(p - glm::vec3(K.x), glm::vec3(0.0), glm::vec3(1.0)), c.y);
}

glm::vec3 rgb2hsv(glm::vec3 rgb)
{
    float h_scale = 60.f;
    float Cmax = std::max(std::max(rgb.x, rgb.y), rgb.z);
    float Cmin = std::min(std::min(rgb.x, rgb.y), rgb.z);
    float delta = Cmax - Cmin;

    float H = 0.0f;
    float S = 0.0f;

    if (delta != 0.0f) {
        if (Cmax == rgb.r) {
            H = (rgb.g - rgb.b) / delta;
        }
        else if (Cmax == rgb.g) {
            H = (rgb.b - rgb.r) / delta + 2.0f;
        }
        else if (Cmax == rgb.b) {
            H = (rgb.r - rgb.g) / delta + 4.0f;
        }
    }

    if (Cmax != 0.0f) {
        S = delta / Cmax;
    }

    H *= h_scale;

    return glm::vec3(H, S, Cmax);
}

glm::vec3 rotate_point_by_quat(const glm::vec3& v, const glm::vec4& q) {
    const glm::vec3 q_vect = glm::vec3(q.x, q.y, q.z);
    return v + 2.0f * glm::cross(q_vect, glm::cross(q_vect, v) + q.w * v);
}

float random_f(float min, float max) {
    return (std::rand() / (RAND_MAX + 1.0)) * (max - min) + min;
}

double random_d(double min, double max)
{
    return (std::rand() / (RAND_MAX + 1.0)) * (max - min) + min;
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

float clamp_rotation(float angle)
{
    float pi2 = 2.0f * glm::pi<float>();
    int turns = static_cast<int>(floor(angle / (pi2)));
    return angle - pi2 * turns;
}

float remap_range(float old_value, float old_min, float old_max, float new_min, float new_max)
{
    return (((old_value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;
}

glm::vec3 yaw_pitch_to_vector(float yaw, float pitch)
{
    return glm::vec3(
        sinf(yaw) * cosf(-pitch),
        sinf(-pitch),
        cosf(yaw) * cosf(-pitch)
    );
}

void vector_to_yaw_pitch(const glm::vec3& front, float* yaw, float* pitch)
{
    *yaw = atan2f(front.x, front.z);
    float mdo = sqrtf(front.x * front.x + front.z * front.z);
    *pitch = atan2f(-front.y, mdo);
}

// Function to compute rotation quaternion to face the camera
glm::quat get_rotation_to_face(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) {
    // Compute direction from object to camera
    glm::vec3 direction = glm::normalize(position - target);

    // Compute rotation quaternion to face the camera
    glm::quat rotation = glm::quatLookAt(direction, up);

    return rotation;
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
    float pi2 = 2.0f * PI;
    if (diff < -PI)
    {
        // lerp upwards past PI_TIMES_TWO
        target += pi2;
        result = smooth_damp(current, target, current_velocity, smooth_time, max_speed, delta_time);
        if (result >= pi2)
        {
            result -= pi2;
        }
    }
    else if (diff > PI)
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
