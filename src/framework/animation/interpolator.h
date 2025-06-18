#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/quaternion.hpp"

#include <vector>
#include <variant>
#include <string>

typedef std::variant<int8_t, int16_t, int32_t, uint8_t, uint16_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::uvec3, glm::uvec4, glm::ivec2, glm::ivec3, glm::ivec4, glm::quat> TrackType;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>

TrackType from_val(const emscripten::val& v);
emscripten::val to_val(const TrackType& v);
#endif

class Keyframe {
public:
    TrackType value;//value
    TrackType in; //in tangent
    TrackType out; //out tangent
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

enum eInterpolationType {
    INTERPOLATION_UNSET,
    INTERPOLATION_STEP,
    INTERPOLATION_LINEAR,
    INTERPOLATION_CUBIC
};

class Interpolator {

    eInterpolationType type = INTERPOLATION_LINEAR;

    TrackType step(const std::vector<Keyframe>& keyframes, int frame_idx, bool looping);
    TrackType linear(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping);
    TrackType cubic(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping);


    /*
    *   Linear interpolation
    */

    TrackType linear_interpolate(TrackType a, TrackType b, float t);

    /*
    *   Cubic spline interpolation
    */

    TrackType hermite(float time, float delta_time, const TrackType& p1, const TrackType& s1, const TrackType& p2, const TrackType& s2);
    TrackType adjust_hermite_result(const TrackType& r);
    void neighborhood(const TrackType& a, TrackType& b);

    template<typename Tn>
    TrackType cubic_interpolate(const TrackType& p1, const TrackType& s1, const TrackType& p2, const TrackType& s2, float h1, float h2, float h3, float h4);

public:

    Interpolator();
    Interpolator(eInterpolationType new_type);

    TrackType interpolate(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping);

    eInterpolationType get_type() const;
    void set_type(eInterpolationType new_type);
};
