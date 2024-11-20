#include "interpolator.h"

#include "glm/gtx/compatibility.hpp"

Interpolator::Interpolator()
{
    type = eInterpolationType::LINEAR;
}

Interpolator::Interpolator(eInterpolationType new_type)
{
    type = new_type;
}

eInterpolationType Interpolator::get_type()
{
    return type;
}

void Interpolator::set_type(eInterpolationType new_type)
{
    type = new_type;
}

TrackType Interpolator::interpolate(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping)
{
    if (type == eInterpolationType::STEP) {
        return step(keyframes, frame_idx, looping);
    }
    else if (type == eInterpolationType::LINEAR) {
        return linear(keyframes, time, frame_idx, looping);
    }
    else if (type == eInterpolationType::CUBIC) {
        return cubic(keyframes, time, frame_idx, looping);
    }

    assert(0);
    return TrackType();
}

TrackType Interpolator::step(const std::vector<Keyframe>& keyframes, int frame_idx, bool looping)
{
    if (frame_idx < 0 || frame_idx >= (int)keyframes.size()) {
        return TrackType();
    }
    return keyframes[frame_idx].value;
}

TrackType Interpolator::linear(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping)
{
    int this_frame = frame_idx;
    int next_frame = this_frame + 1;
    if (this_frame < 0 || this_frame > keyframes.size() - 1) {
        return TrackType();
    }

    if (this_frame == keyframes.size() - 1) {
        return keyframes[this_frame].value;
    }

    float this_time = keyframes[this_frame].time;
    float frame_delta = keyframes[next_frame].time - this_time;
    if (frame_delta <= 0.0f) {
        return TrackType();
    }

    float t = (time - this_time) / frame_delta;
    TrackType start = keyframes[this_frame].value;
    TrackType end = keyframes[next_frame].value;

    return linear_interpolate(start, end, t);
}

TrackType Interpolator::cubic(const std::vector<Keyframe>& keyframes, float time, int frame_idx, bool looping)
{
    int this_frame = frame_idx;
    int next_frame = this_frame + 1;
    if (this_frame < 0 || this_frame > keyframes.size() - 1) {
        return TrackType();
    }
    if (this_frame == keyframes.size() - 1) {
        return keyframes[this_frame].value;
    }

    float this_time = keyframes[this_frame].time;
    float frame_delta = keyframes[next_frame].time - this_time;
    if (frame_delta <= 0.0f) {
        return TrackType();
    }

    float t = (time - this_time) / frame_delta;

    return hermite(t, frame_delta,
        keyframes[this_frame].value, keyframes[this_frame].out,
        keyframes[next_frame].value, keyframes[next_frame].in
    );
}

TrackType Interpolator::linear_interpolate(TrackType a, TrackType b, float t)
{
    if (std::holds_alternative<float>(a))
    {
        float qa = std::get<float>(a);
        float qb = std::get<float>(b);
        return std::get<float>(a) + (std::get<float>(b) - std::get<float>(a)) * t;
    }
    else if (std::holds_alternative<glm::vec3>(a)) {
        return lerp(std::get<glm::vec3>(a), std::get<glm::vec3>(b), t);
    }
    else if (std::holds_alternative<glm::vec4>(a)) {
        return lerp(std::get<glm::vec4>(a), std::get<glm::vec4>(b), t);
    }
    else if (std::holds_alternative<glm::quat>(a)) {
        glm::quat qa = std::get<glm::quat>(a);
        glm::quat qb = std::get<glm::quat>(b);

        glm::quat result = mix(qa, qb, t);
        if (dot(qa, qb) < 0) { // Neighborhood
            result = mix(qa, -qb, t);
        }
        return normalize(result); // NLerp, not slerp
    }

    assert(0);
    return TrackType();
}

template<typename Tn>
TrackType Interpolator::cubic_interpolate(const TrackType& p1, const TrackType& s1, const TrackType& p2, const TrackType& s2, float h1, float h2, float h3, float h4)
{
    return std::get<Tn>(p1) * h1 + std::get<Tn>(p2) * h2 + std::get<Tn>(s1) * h3 + std::get<Tn>(s2) * h4;
}

TrackType Interpolator::hermite(float t, float delta_time, const TrackType& _p1, const TrackType& _s1, const TrackType& _p2, const TrackType& _s2)
{
    float tt = t * t;
    float ttt = tt * t;
    TrackType p2 = _p2;
    neighborhood(_p1, p2);
    float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    float h2 = -2.0f * ttt + 3.0f * tt;
    float h3 = ttt - 2.0f * tt + t;
    float h4 = ttt - tt;

    TrackType result;

    if (std::holds_alternative<float>(_p1)) {
        float s1 = std::get<float>(_s1) * delta_time;
        float s2 = std::get<float>(_s2) * delta_time;
        result = cubic_interpolate<float>(_p1, s1, p2, s2, h1, h2, h3, h4);
    }
    else if (std::holds_alternative<glm::vec3>(_p1)) {
        glm::vec3 s1 = std::get<glm::vec3>(_s1) * delta_time;
        glm::vec3 s2 = std::get<glm::vec3>(_s2) * delta_time;
        result = cubic_interpolate<glm::vec3>(_p1, s1, p2, s2, h1, h2, h3, h4);
    }
    else if (std::holds_alternative<glm::quat>(_p1)) {
        glm::quat s1 = std::get<glm::quat>(_s1) * delta_time;
        glm::quat s2 = std::get<glm::quat>(_s2) * delta_time;
        result = cubic_interpolate<glm::quat>(_p1, s1, p2, s2, h1, h2, h3, h4);
    }

    return adjust_hermite_result(result);
}

TrackType Interpolator::adjust_hermite_result(const TrackType& r)
{
    if (std::holds_alternative<glm::quat>(r)) {
        glm::quat q = std::get<glm::quat>(r);
        return normalize(q);
    }

    return r;
}

void Interpolator::neighborhood(const TrackType& a, TrackType& b)
{
    if (!std::holds_alternative<glm::quat>(a)) {
        return;
    }

    glm::quat qa = std::get<glm::quat>(a);
    glm::quat qb = std::get<glm::quat>(b);

    if (dot(qa, qb) < 0) {
        b = -qb;
    }
}
