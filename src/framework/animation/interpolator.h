#pragma once

#include "glm/gtc/quaternion.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <string>
#include <variant>
#include <vector>

typedef std::variant<int8_t, int16_t, int32_t, uint8_t, uint16_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::uvec3, glm::uvec4, glm::ivec2, glm::ivec3, glm::ivec4, glm::quat> TrackT;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

TrackT from_val(const emscripten::val& v);
emscripten::val to_val(const TrackT& v);
#endif

class Keyframe {
public:
    TrackT value; //value
    TrackT in; //in tangent
    TrackT out; //out tangent
    float time; //frame time

#ifdef __EMSCRIPTEN__
    emscripten::val get_value() const { return to_val(value); }
    void set_value(emscripten::val v) { value = from_val(v); }

    emscripten::val get_in() const { return to_val(in); }
    void set_in(emscripten::val v) { in = from_val(v); }

    emscripten::val get_out() const { return to_val(out); }
    void set_out(emscripten::val v) { out = from_val(v); }
#endif
};

enum InterpolationType {
    INTERPOLATION_UNSET,
    INTERPOLATION_STEP,
    INTERPOLATION_LINEAR,
    INTERPOLATION_CUBIC
};

class Interpolator {
    InterpolationType type = INTERPOLATION_LINEAR;

    TrackT step(const std::vector<Keyframe>& keyframes, int frame_idx, bool looping);
    TrackT linear(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping);
    TrackT cubic(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping);

    /*
     *   Linear interpolation
     */

    TrackT linear_interpolate(TrackT a, TrackT b, float t);

    /*
     *   Cubic spline interpolation
     */

    TrackT hermite(float time, float delta_time, const TrackT& p1, const TrackT& s1, const TrackT& p2, const TrackT& s2);
    TrackT adjust_hermite_result(const TrackT& r);
    void neighborhood(const TrackT& a, TrackT& b);

    template <typename Tn>
    TrackT cubic_interpolate(const TrackT& p1, const TrackT& s1, const TrackT& p2, const TrackT& s2, float h1, float h2, float h3, float h4);

public:
    Interpolator();
    Interpolator(InterpolationType new_type);

    TrackT interpolate(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping);

    InterpolationType get_type() const;
    void set_type(InterpolationType new_type);
};
