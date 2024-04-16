#pragma once
#include <vector>
#include <string>
#include <iostream>

#include "track.h"
#include "pose.h"

enum AnimationType {
    ANIM_TYPE_UNDEFINED,
    ANIM_TYPE_SIMPLE,
    ANIM_TYPE_SKELETON,
};

class Animation;

class Animation {

    std::vector<Track> tracks;
    std::string name;

    float start_time = 0.0f;
    float end_time = 0.0f;
    bool looping = true;

    float adjust_time_to_fit_range(float time);
    void sample_pose(float time, void* out);

    AnimationType type = ANIM_TYPE_UNDEFINED;

public:

    Animation();
    ~Animation() {};

    // Gets joint Id based for a specific track index
    uint32_t get_id_at_index(uint32_t index);
    // Sets joint ID based on the index of the joint in the clip
    void set_id_at_index(uint32_t idx, uint32_t id);
    uint32_t size();

    // Samples the animation clip at the provided time into the out reference
    float sample(float time, void* out = nullptr);

    // Returns a transform track for the specified track position id
    Track* operator[](uint32_t index);

    // Sets the start/end time of the animation clip based on the tracks that make up the clip
    void recalculate_duration();

    // Adds a new track
    Track* add_track(uint32_t id);

    Track* get_track_by_id(uint32_t id);
    Track* get_track(uint32_t index);
    std::string& get_name();
    float get_duration();
    float get_start_time();
    float get_end_time();
    bool get_looping();
    AnimationType get_type();

    void set_type(AnimationType new_type);
    void set_name(const std::string& new_name);
    void set_looping(bool new_looping);
};


