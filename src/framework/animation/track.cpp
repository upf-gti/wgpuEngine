#include "track.h"

template Track<float, 1>;
template Track<glm::vec3, 3>;
template Track<glm::quat, 4>;

// This class is templated, so some functions are declared but not necessary implemented depending of the interpolation type

// Track helpers avoid having to make specialized versions of the interpolation functions
namespace TrackHelpers {

    // linear interpolation for each type of track
    inline float interpolate(float a, float b, float t) {
        return a + (b - a) * t;
    }
    inline glm::vec3 interpolate(const glm::vec3& a, const glm::vec3& b, float t) {
        return lerp(a, b, t);
    }
    inline glm::quat interpolate(const glm::quat& a, const glm::quat& b, float t) {
        glm::quat result = mix(a, b, t);
        if (dot(a, b) < 0) { // Neighborhood
            result = mix(a, -b, t);
        }
        return normalize(result); // NLerp, not slerp
    }

    // When a Hermite spline is interpolated, if the input type was a quaternion, the result needs to be normalized.
    inline float adjustHermiteResult(float f) {
        return f;
    }
    inline glm::vec3 adjustHermiteResult(const glm::vec3& v) {
        return v;
    }
    inline glm::quat adjustHermiteResult(const glm::quat& q) {
        return normalize(q);
    }

    // Make sure two quaternions are in the correct neighborhood
    inline void neighborhood(const float& a, float& b) { }
    inline void neighborhood(const glm::vec3& a, glm::vec3& b) { }
    inline void neighborhood(const glm::quat& a, glm::quat& b) {
        if (dot(a, b) < 0) {
            b = -b;
        }
    }
}; // End Track Helpers namespace

template<typename T, int N>
Track<T, N>::Track() {
    interpolation = Interpolation::Linear;
}

template<typename T, int N>
float Track<T, N>::get_start_time() {
    return frames[0].time;
}

template<typename T, int N>
float Track<T, N>::get_end_time() {
    return frames[frames.size() - 1].time;
}

// call sample_constant, sample_linear, or sample_cubic, depending on the track type.
template<typename T, int N>
T Track<T, N>::sample(float time, bool looping) {
    if (interpolation == Interpolation::Constant) {
        return sample_constant(time, looping);
    }
    else if (interpolation == Interpolation::Linear) {
        return sample_linear(time, looping);
    }

    return sample_cubic(time, looping);
}

template<typename T, int N>
Frame<N>& Track<T, N>::operator[](unsigned int index) {
    return frames[index];
}

// size of the frames vector
template<typename T, int N>
void Track<T, N>::resize(unsigned int size) {
    frames.resize(size);
}

template<typename T, int N>
unsigned int Track<T, N>::size() {
    return frames.size();
}

template<typename T, int N>
Interpolation Track<T, N>::get_interpolation() {
    return interpolation;
}

template<typename T, int N>
void Track<T, N>::set_interpolation(Interpolation interpolationType) {
    interpolation = interpolationType;
}

template<typename T, int N>
T Track<T, N>::hermite(float t, const T& p1, const T& s1, const T& _p2, const T& s2) {
    float tt = t * t;
    float ttt = tt * t;
    T p2 = _p2;
    TrackHelpers::neighborhood(p1, p2);
    float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    float h2 = -2.0f * ttt + 3.0f * tt;
    float h3 = ttt - 2.0f * tt + t;
    float h4 = ttt - tt;
    T result = p1 * h1 + p2 * h2 + s1 * h3 + s2 * h4;
    return TrackHelpers::adjustHermiteResult(result);
}

// return the frame immediately before that time (on the left)
template<typename T, int N>
int Track<T, N>::frame_index(float time, bool looping) {
    unsigned int size = (unsigned int)frames.size();
    if (size <= 1) {
        return -1;
    }
    // If the track is sampled as looping, the input time needs to be adjusted so that it falls between the start and end frames.
    if (looping) {
        float startTime = frames[0].time;
        float endTime = frames[size - 1].time;
        float duration = endTime - startTime;
        time = fmodf(time - startTime, endTime - startTime);
        // looping, time needs to be adjusted so that it is within a valid range.
        if (time < 0.0f) {
            time += endTime - startTime;
        }
        time = time + startTime;
    }
    else {
        // clamp the time in the track frames range
        if (time <= frames[0].time) {
            return 0;
        }
        if (time >= frames[size - 2].time) {
            // The Sample function always needs a current and next frame (for interpolation), so the index of the second-to-last frame is used.
            return (int)size - 2;
        }
    }
    for (int i = (int)size - 1; i >= 0; --i) {
        if (time >= frames[i].time) {
            return i;
        }
    }
    // Invalid code, we should not reach here!
    return -1;
} // End of frame_index

// Adjusts the time to be in the range of the start/end frames of the track.
template<typename T, int N>
float Track<T, N>::adjust_time_to_fit_track(float time, bool looping) {
    unsigned int size = (unsigned int)frames.size();
    if (size <= 1) {
        return 0.0f;
    }
    float startTime = frames[0].time;
    float endTime = frames[size - 1].time;
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
        if (time <= frames[0].time) {
            time = startTime;
        }
        if (time >= frames[size - 1].time) {
            time = endTime;
        }
    }
    return time;
}

// They cast a float array stored in a Frame class into the data type that the Frame class represents
template<> float Track<float, 1>::cast(float* value) {
    return value[0];
}

template<> glm::vec3 Track<glm::vec3, 3>::cast(float* value) {
    return glm::vec3(value[0], value[1], value[2]);
}

template<> glm::quat Track<glm::quat, 4>::cast(float* value) {
    glm::quat r = glm::quat(value[0], value[1], value[2], value[3]);
    return normalize(r);
}

// often used for things such as visibility flags, where it makes sense for the value of a variable to change from one frame to the next without any real interpolation
template<typename T, int N>
T Track<T, N>::sample_constant(float t, bool loop) {
    int frame = frame_index(t, loop);
    if (frame < 0 || frame >= (int)frames.size()) {
        return T();
    }
    return cast(&frames[frame].value[0]);
}

// applications provide an option to approximate animation curves by sampling them at set intervals
template<typename T, int N>
T Track<T, N>::sample_linear(float time, bool looping) {
    int thisFrame = frame_index(time, looping);
    if (thisFrame < 0 || thisFrame >= frames.size() - 1) {
        return T();
    }
    int nextFrame = thisFrame + 1;
    // make sure the time is valid
    float trackTime = adjust_time_to_fit_track(time, looping);
    float thisTime = frames[thisFrame].time;
    float frameDelta = frames[nextFrame].time - thisTime;
    if (frameDelta <= 0.0f) {
        return T();
    }
    float t = (trackTime - thisTime) / frameDelta;
    T start = cast(&frames[thisFrame].value[0]);
    T end = cast(&frames[nextFrame].value[0]);
    return TrackHelpers::interpolate(start, end, t);
}

template<typename T, int N>
T Track<T, N>::sample_cubic(float time, bool looping) {
    int thisFrame = frame_index(time, looping);
    if (thisFrame < 0 || thisFrame >= frames.size() - 1) {
        return T();
    }
    int nextFrame = thisFrame + 1;

    float trackTime = adjust_time_to_fit_track(time, looping);
    float thisTime = frames[thisFrame].time;
    float frameDelta = frames[nextFrame].time - thisTime;
    if (frameDelta <= 0.0f) {
        return T();
    }

    // cast function normalizes quaternions, which is bad because slopes are not meant to be quaternions.
    // Using memcpy instead of cast copies the values directly, avoiding normalization.
    float t = (trackTime - thisTime) / frameDelta;
    size_t fltSize = sizeof(float);
    T point1 = cast(&frames[thisFrame].value[0]);
    T slope1;// = frames[thisFrame].out * frameDelta;
    memcpy(&slope1, frames[thisFrame].out, N * fltSize); // memcpy instead of cast to avoid normalization
    slope1 = slope1 * frameDelta;
    T point2 = cast(&frames[nextFrame].value[0]);
    T slope2;// = frames[nextFrame].in[0] * frameDelta;
    memcpy(&slope2, frames[nextFrame].in, N * fltSize);
    slope2 = slope2 * frameDelta;

    return hermite(t, point1, slope1, point2, slope2);
}
