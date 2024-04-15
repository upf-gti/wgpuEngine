#include "track.h"

#include <algorithm>

// Track helpers avoid having to make specialized versions of the interpolation functions
namespace TrackHelpers {
   
    // linear interpolation for each type of track
    T interpolate(T a, T b, float t) {
        if (std::holds_alternative<float>(a))
        {
            float qa = std::get<float>(a);
            float qb = std::get<float>(b);
            return std::get<float>(a) + (std::get<float>(b) - std::get<float>(a)) * t;
        }
        else if (std::holds_alternative<glm::vec3>(a)) {
            return lerp(std::get<glm::vec3>(a), std::get<glm::vec3>(b), t);
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
        return T();
    }
    
    // When a Hermite spline is interpolated, if the input type was a quaternion, the result needs to be normalized.
    T adjustHermiteResult(const T& r) {
        if (std::holds_alternative<glm::quat>(r))
        {
            glm::quat q = std::get<glm::quat>(r);
            return normalize(q);
        }
        else {
            return r;
        }
    }

    // Make sure two quaternions are in the correct neighborhood
    void neighborhood(const T& a, T& b) {
        
        if (std::holds_alternative<glm::quat>(a))
        {
            glm::quat qa = std::get<glm::quat>(a);
            glm::quat qb = std::get<glm::quat>(b);

            if (dot(qa, qb) < 0) {
                b = -qb;
            }
        }
    }

    uint32_t get_size(const Track& t) {
        switch (t.get_type()) {

        case TrackType::TYPE_FLOAT:
            return 1;

        case TrackType::TYPE_VECTOR3:
            return 3;

        case TrackType::TYPE_QUAT:
            return 4;
        }
        return 0;
    }
}; // End Track Helpers namespace

Track::Track()
{
    interpolation = Interpolation::LINEAR;
}

uint32_t Track::get_id()
{
    return id;
}

void Track::set_data(void* data)
{
    this->data = data;
}

void Track::set_id(uint32_t index)
{
    id = index;
}

void Track::set_name(const std::string& n)
{
    name = n;
}

float Track::get_start_time()
{
    return keyframes[0].time;
}

float Track::get_end_time()
{
    return keyframes[keyframes.size() - 1].time;
}

std::string& Track::get_name()
{
    return name;
}

void* Track::get_data()
{
    return data;
}
// call sample_constant, sample_linear, or sample_cubic, depending on the track type.
T Track::sample(float time, bool looping, void *out)
{
    T r;

    if (interpolation == Interpolation::CONSTANT) {
        r = sample_constant(time, looping);
    }
    else if (interpolation == Interpolation::LINEAR) {
        r = sample_linear(time, looping);
    }
    else {
        r = sample_cubic(time, looping);
    }

    // Update property data of the track
    if (std::holds_alternative<float>(r)) {
        float* p = reinterpret_cast<float*>(out);
        *p = std::get<float>(r);
    }
    else if (std::holds_alternative<glm::vec3>(r)) {
        glm::vec3* p = reinterpret_cast<glm::vec3*>(out);
        *p = std::get<glm::vec3>(r);
    }
    else if (std::holds_alternative<glm::quat>(r)) {
        glm::quat* p = reinterpret_cast<glm::quat*>(out);
        *p = std::get<glm::quat>(r);
    }
    
    /*if (data)
    {
        if (std::holds_alternative<float>(r)) {
            float* p = reinterpret_cast<float*>(data);
            *p = std::get<float>(r);
        }
        else if (std::holds_alternative<glm::vec3>(r)) {
            glm::vec3* p = reinterpret_cast<glm::vec3*>(data);
            *p = std::get<glm::vec3>(r);
        }
        else if (std::holds_alternative<glm::quat>(r)) {
            glm::quat* p = reinterpret_cast<glm::quat*>(data);
            *p = std::get<glm::quat>(r);
        }
    }*/

    return r;
}

Keyframe& Track::operator[](uint32_t index)
{
    return keyframes[index];
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

Interpolation Track::get_interpolation()
{
    return interpolation;
}

void Track::set_interpolation(Interpolation interpolationType)
{
    interpolation = interpolationType;
}

template<typename Tn>
T Track::cubic_interpolation(const T& p1, const T& s1, const T& p2, const T& s2, float h1, float h2, float h3, float h4)
{
    return std::get<Tn>(p1) * h1 + std::get<Tn>(p2) * h2 + std::get<Tn>(s1) * h3 + std::get<Tn>(s2) * h4;
}

T Track::hermite(float t, const T& p1, const T& s1, const T& _p2, const T& s2)
{
    float tt = t * t;
    float ttt = tt * t;
    T p2 = _p2;
    TrackHelpers::neighborhood(p1, p2);
    float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    float h2 = -2.0f * ttt + 3.0f * tt;
    float h3 = ttt - 2.0f * tt + t;
    float h4 = ttt - tt;

    T result;

    if (std::holds_alternative<float>(p1)) {
        result = cubic_interpolation<float>(p1, s1, p2, s2, h1, h2, h3, h4);
    }
    else if (std::holds_alternative<glm::vec3>(p1)) {
        result = cubic_interpolation<glm::vec3>(p1, s1, p2, s2, h1, h2, h3, h4);
    }
    else if (std::holds_alternative<glm::quat>(p1)) {
        result = cubic_interpolation<glm::quat>(p1, s1, p2, s2, h1, h2, h3, h4);
    }

    return TrackHelpers::adjustHermiteResult(result);
}

// Return the frame immediately before that time (on the left)
int Track::frame_index(float time, bool looping)
{
    uint32_t size = (uint32_t)keyframes.size();

    if (size <= 1) {
        return -1;
    }

    // If the track is sampled as looping, the input time needs to be adjusted so that it falls between the start and end keyframes.
    if (looping) {
        float startTime = keyframes[0].time;
        float endTime = keyframes[size - 1].time;
        float duration = endTime - startTime;
        time = fmodf(time - startTime, endTime - startTime);
        // looping, time needs to be adjusted so that it is within a valid range.
        if (time < 0.0f) {
            time += endTime - startTime;
        }
        time = time + startTime;
    }
    else {
        // clamp the time in the track keyframes range
        if (time <= keyframes[0].time) {
            return 0;
        }
        if (time >= keyframes[size - 2].time) {
            // The Sample function always needs a current and next frame (for interpolation), so the index of the second-to-last frame is used.
            return (int)size - 2;
        }
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
    float startTime = keyframes[0].time;
    float endTime = keyframes[size - 1].time;
    float duration = endTime - startTime;
    if (duration <= 0.0f) {
        return 0.0f;
    }
    if (looping) {
        time = fmodf(time - startTime, endTime - startTime);
        if (time < 0.0f) {
            time += endTime - startTime;
        }
        time = time + startTime;
    }

    else {
        if (time <= keyframes[0].time) {
            time = startTime;
        }
        if (time >= keyframes[size - 1].time) {
            time = endTime;
        }
    }
    return time;
}

// Often used for things such as visibility flags, where it makes sense for the value of a variable to change from one frame to the next without any real interpolation
T Track::sample_constant(float t, bool loop)
{
    int frame = frame_index(t, loop);
    if (frame < 0 || frame >= (int)keyframes.size()) {
        assert(0);
    }
    return keyframes[frame].value;
}

// Applications provide an option to approximate animation curves by sampling them at set intervals
T Track::sample_linear(float time, bool looping)
{
    int this_frame = frame_index(time, looping);
    if (this_frame < 0 || this_frame >= keyframes.size() - 1) {
        assert(0);
    }
    int next_frame = this_frame + 1;
    // make sure the time is valid
    float track_time = adjust_time_to_fit_track(time, looping);
    float this_time = keyframes[this_frame].time;
    float frame_delta = keyframes[next_frame].time - this_time;
    if (frame_delta <= 0.0f) {
        assert(0);
    }
    float t = (track_time - this_time) / frame_delta;
    T start = keyframes[this_frame].value;
    T end = keyframes[next_frame].value;
    return TrackHelpers::interpolate(start, end, t);
}

T Track::sample_cubic(float time, bool looping)
{
    int this_frame = frame_index(time, looping);
    if (this_frame < 0 || this_frame >= keyframes.size() - 1) {
        assert(0);
    }
    int next_frame = this_frame + 1;

    float track_time = adjust_time_to_fit_track(time, looping);
    float this_time = keyframes[this_frame].time;
    float frame_delta = keyframes[next_frame].time - this_time;

    if (frame_delta <= 0.0f) {
        assert(0);
    }

    // auto mul_T = std::multiplies<T>();

    // cast function normalizes quaternions, which is bad because slopes are not meant to be quaternions.
    // Using memcpy instead of cast copies the values directly, avoiding normalization.
    float t = (track_time - this_time) / frame_delta;
    size_t fltSize = sizeof(float);
    T point1 = keyframes[this_frame].value;
    /*T slope1 = mul_T(keyframes[this_frame].out, frame_delta);
    
    T point2 =keyframes[next_frame].value;
    T slope2 = mul_T(keyframes[next_frame].in, frame_delta);*/

    //memcpy(&slope1, keyframes[this_frame].out, N * fltSize); // memcpy instead of cast to avoid normalization
   // memcpy(&slope2, keyframes[next_frame].in, N * fltSize);

    return point1;// hermite(t, point1, slope1, point2, slope2);
}
