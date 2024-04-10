#pragma once

#include "track.h"
#include "bone_transform.h"

// binds a transform to a joint
class TransformTrack {
protected:
    unsigned int id; // joint Id
    VectorTrack position;
    QuaternionTrack rotation;
    VectorTrack scale;
public:
    TransformTrack();
    unsigned int get_id();
    void set_id(unsigned int id);
    VectorTrack& get_position_track();
    QuaternionTrack& get_rotation_track();
    VectorTrack& get_scale_track();
    float get_start_time();
    float get_end_time();
    bool is_valid();
    Transform sample(const Transform& ref, float time, bool looping);
};

