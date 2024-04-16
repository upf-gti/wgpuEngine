#pragma once

#include "framework/math.h"

#include <vector>
#include <variant>
#include <string>

enum SampleMode {
    SAMPLE_UPDATE,
    SAMPLE_RETURN
};

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
    TYPE_POSITION,
    TYPE_ROTATION,
    TYPE_SCALE
};

// Collection of keyframes
class Track {

    uint32_t id = 0;
    std::vector<Keyframe> keyframes;
    Interpolation interpolation; // interpolation type
    TrackType type = TrackType::TYPE_UNDEFINED;
    std::string name = "";
    void* data = nullptr;

    // Helper functions, a sample for each type of interpolation
    T sample_constant(float time, bool looping);
    T sample_linear(float time, bool looping);
    T sample_cubic(float time, bool looping);

    // Helper function to evaluate Hermite splines (tangents)
    T hermite(float time, const T& p1, const T& s1, const T& p2, const T& s2);

    template<typename Tn>
    T cubic_interpolation(const T& p1, const T& s1, const T& p2, const T& s2, float h1, float h2, float h3, float h4);

    int frame_index(float time, bool looping);

    // Takes an input time that is outside the range of the track and adjusts it to be a valid time on the track
    float adjust_time_to_fit_track(float time, bool loop);

public:

    Track();

    uint32_t get_id();
    float get_end_time();
    float get_start_time();
    Interpolation get_interpolation();
    TrackType get_type() const { return type; };
    std::string& get_name();
    void* get_data();

    void set_data(void* property);
    void set_id(uint32_t id);
    void set_interpolation(Interpolation interp);
    void set_type(TrackType new_type) { type = new_type; };
    void set_name(const std::string& name);

    uint32_t size();
    void resize(uint32_t size);

    // Prameters: time value, if the track is looping or not
    T sample(float time, bool looping, SampleMode mode);
    Keyframe& operator[](uint32_t index);
};

namespace TrackHelpers {
    T interpolate(T a, T b, float t);
    T adjustHermiteResult(const T& r);
    void neighborhood(const T& a, T& b);
    uint32_t get_size(const Track& t);
}
