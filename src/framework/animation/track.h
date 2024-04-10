#pragma once

#include "frame.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include <vector>

enum class Interpolation {
    Constant,
    Linear,
    Cubic
};

// Collection of frames
template<typename T, int N>
class Track {
protected:
    std::vector<Frame<N>> frames;
    Interpolation interpolation; // interpolation type
public:
    Track();
    void resize(unsigned int size);
    unsigned int size();
    Interpolation get_interpolation();
    void set_interpolation(Interpolation interp);
    float get_start_time();
    float get_end_time();
    // parameters: time value, if the track is looping or not
    T sample(float time, bool looping);
    Frame<N>& operator[](unsigned int index);
protected:
    // helper functions, a sample for each type of interpolation
    T sample_constant(float time, bool looping);
    T sample_linear(float time, bool looping);
    T sample_cubic(float time, bool looping);
    // helper function to evaluate Hermite splines (tangents)
    T hermite(float time, const T& p1, const T& s1, const T& p2, const T& s2);
    int frame_index(float time, bool looping);
    // takes an input time that is outside the range of the track and adjusts it to be a valid time on the track
    float adjust_time_to_fit_track(float t, bool loop);

    // casts an array of floats (the data inside a frame) to the templated type of the track	
    T cast(float* value); // Will be specialized
};

typedef Track<float, 1> ScalarTrack;
typedef Track<glm::vec3, 3> VectorTrack;
typedef Track<glm::quat, 4> QuaternionTrack;
