#include "track.h"

#include <algorithm>
#include "glm/gtx/compatibility.hpp"

Track::Track()
{
    
}

int Track::get_id()
{
    return id;
}

void Track::set_id(int index)
{
    id = index;
}

void Track::set_name(const std::string& new_name)
{
    name = new_name;
}

void Track::set_path(const std::string& new_path)
{
    path = new_path;
}

float Track::get_start_time()
{
    return keyframes[0].time;
}

float Track::get_end_time()
{
    return keyframes[keyframes.size() - 1].time;
}

const std::string& Track::get_name()
{
    return name;
}

const std::string& Track::get_path()
{
    return path;
}

uint8_t Track::get_stride()
{
    switch (type) {

    case eTrackType::TYPE_FLOAT:
        return 1;

    case eTrackType::TYPE_VECTOR3: case eTrackType::TYPE_POSITION: case eTrackType::TYPE_SCALE:
        return 3;

    case eTrackType::TYPE_QUAT: case eTrackType::TYPE_ROTATION:
        return 4;
    }

    assert(0);

    return 0;
}

Keyframe& Track::get_keyframe(uint32_t index)
{
    if (index >= keyframes.size()) {
        assert(0);
    }

    return keyframes[index];
}

Keyframe& Track::add_keyframe(const Keyframe& k)
{
    keyframes.push_back(k);
    return keyframes.back();
}

// call sample_constant, sample_linear, or sample_cubic, depending on the track type.
TrackType Track::sample(float time, bool looping, void* out, eInterpolationType interpolation_type)
{
    float track_time = adjust_time_to_fit_track(time, looping);
    int frame_idx = frame_index(time);

    eInterpolationType prev_type = interpolator.get_type();

    if (interpolation_type != eInterpolationType::UNSET) {
        interpolator.set_type(interpolation_type);
    }

    TrackType r = interpolator.interpolate(keyframes, track_time, frame_idx, looping);

    interpolator.set_type(prev_type);

    if (out)
    {
        // TODO: Support the rest of types..

        if (std::holds_alternative<float>(r)) {
            float* p = reinterpret_cast<float*>(out);
            *p = std::get<float>(r);
        }
        else if (std::holds_alternative<glm::vec3>(r)) {
            glm::vec3* p = reinterpret_cast<glm::vec3*>(out);
            *p = std::get<glm::vec3>(r);
        }
        else if (std::holds_alternative<glm::vec4>(r)) {
            glm::vec4* p = reinterpret_cast<glm::vec4*>(out);
            *p = std::get<glm::vec4>(r);
        }
        else if (std::holds_alternative<glm::quat>(r)) {
            glm::quat* p = reinterpret_cast<glm::quat*>(out);
            *p = std::get<glm::quat>(r);
        }
    }

    return r;
}

Keyframe& Track::operator[](uint32_t index)
{
    return get_keyframe(index);
}

// Size of the keyframes vector
void Track::resize(uint32_t size)
{
    keyframes.resize(size);
}

uint32_t Track::size()
{
    return (uint32_t)keyframes.size();
}

// Return the frame immediately before that time (on the left)
int Track::frame_index(float time)
{
    uint32_t size = (uint32_t)keyframes.size();

    if (size <= 1) {
        return -1;
    }

    for (int i = (int)size - 1; i >= 0; --i) {
        if (time >= keyframes[i].time) {
            return i;
        }
    }

    // Invalid code, we should not reach here!
    return -1;
}

// Adjusts the time to be in the range of the start/end keyframes of the track.
float Track::adjust_time_to_fit_track(float time, bool looping)
{
    uint32_t size = (uint32_t)keyframes.size();

    if (size <= 1) {
        return 0.0f;
    }

    float start_time = keyframes[0].time;
    float end_time = keyframes[size - 1].time;
    float duration = end_time - start_time;

    if (duration <= 0.0f) {
        return 0.0f;
    }

    if (looping) {
        time = fmodf(time - start_time, end_time - start_time);
        if (time < 0.0f) {
            time += end_time - start_time;
        }
        time = time + start_time;
    }
    else {
        if (time <= keyframes[0].time) {
            time = start_time;
        }
        if (time >= keyframes[size - 1].time) {
            time = end_time;
        }
    }

    return time;
}
