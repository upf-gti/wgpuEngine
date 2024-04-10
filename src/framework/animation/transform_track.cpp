#include "transform_track.h"

TransformTrack::TransformTrack() {
    id = 0;
}

unsigned int TransformTrack::get_id() {
    return id;
}

void TransformTrack::set_id(unsigned int index) {
    id = index;
}

VectorTrack& TransformTrack::get_position_track() {
    return position;
}

QuaternionTrack& TransformTrack::get_rotation_track() {
    return rotation;
}

VectorTrack& TransformTrack::get_scale_track() {
    return scale;
}

bool TransformTrack::is_valid() {
    return position.size() > 1 ||
        rotation.size() > 1 ||
        scale.size() > 1;
}

float TransformTrack::get_start_time() {
    float result = 0.0f;
    bool isSet = false;
    if (position.size() > 1) {
        result = position.get_start_time();
        isSet = true;
    }
    if (rotation.size() > 1) {
        float rotationStart = rotation.get_start_time();
        if (rotationStart < result || !isSet) {
            result = rotationStart;
            isSet = true;
        }
    }
    if (scale.size() > 1) {
        float scaleStart = scale.get_start_time();
        if (scaleStart < result || !isSet) {
            result = scaleStart;
            isSet = true;
        }
    }
    return result;
}

float TransformTrack::get_end_time() {
    float result = 0.0f;
    bool isSet = false;
    if (position.size() > 1) {
        result = position.get_end_time();
        isSet = true;
    }
    if (rotation.size() > 1) {
        float rotationEnd = rotation.get_end_time();
        if (rotationEnd > result || !isSet) {
            result = rotationEnd;
            isSet = true;
        }
    }
    if (scale.size() > 1) {
        float scaleEnd = scale.get_end_time();
        if (scaleEnd > result || !isSet) {
            result = scaleEnd;
            isSet = true;
        }
    }
    return result;
}

// only samples one of its component tracks if that track has two or more frames
Transform TransformTrack::sample(const Transform& ref, float time, bool loop) {
    // If one of the transform components isn't animated by the transform track, the value of the reference transform is used 
    Transform result = ref; // Assign default values
    if (position.size() > 1) { // Only if valid
        result.position = position.sample(time, loop);
    }
    if (rotation.size() > 1) { // Only if valid
        result.rotation = rotation.sample(time, loop);
    }
    if (scale.size() > 1) { // Only if valid
        result.scale = scale.sample(time, loop);
    }
    return result;
}
