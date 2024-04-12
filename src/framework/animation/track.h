#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include <vector>
#include <variant>

enum class Interpolation {
    CONSTANT,
    LINEAR,
    CUBIC
};

typedef std::variant<float, uint8_t, uint16_t, glm::vec3, glm::quat> T;

class Keyframe {
public:
    T value;//value
    T in; //in tangent
    T out; //out tangent
    float time; //frame time
};

enum TrackType {
    TYPE_UNDEFINED,
    TYPE_FLOAT, // Set a value in a property, can be interpolated.
    TYPE_VECTOR3, // Vector3 track, can be compressed.
    TYPE_QUAT, // Quaternion track, can be compressed.
    TYPE_METHOD, // Call any method on a specific node.
};

// Collection of keyframes
class Track {

    uint32_t id = 0; // id
    std::vector<Keyframe> keyframes;
    Interpolation interpolation; // interpolation type
    TrackType type = TrackType::TYPE_UNDEFINED;

    void* data = nullptr;

    // helper functions, a sample for each type of interpolation
    T sample_constant(float time, bool looping);
    T sample_linear(float time, bool looping);
    T sample_cubic(float time, bool looping);
    // helper function to evaluate Hermite splines (tangents)

    template<typename Tn>
    T cubic_interpolation(const T& p1, const T& s1, const T& p2, const T& s2, float h1, float h2, float h3, float h4);

    T hermite(float time, const T& p1, const T& s1, const T& p2, const T& s2);
    int frame_index(float time, bool looping);
    // takes an input time that is outside the range of the track and adjusts it to be a valid time on the track
    float adjust_time_to_fit_track(float time, bool loop);

public:

    Track();

    uint32_t get_id();
    float get_end_time();
    float get_start_time();
    Interpolation get_interpolation();
    TrackType get_type() const { return type; };

    void set_data(void* property);
    void set_id(uint32_t id);
    void set_interpolation(Interpolation interp);
    void set_type(TrackType new_type) { type = new_type; };

    uint32_t size();
    void resize(uint32_t size);
    // parameters: time value, if the track is looping or not
    T sample(float time, bool looping);
    Keyframe& operator[](uint32_t index);
};

namespace TrackHelpers {
    T interpolate(T a, T b, float t);
    T adjustHermiteResult(const T& r);
    void neighborhood(const T& a, T& b);
    uint32_t get_size(const Track& t);
}
