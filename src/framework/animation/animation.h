#pragma once
#include <vector>
#include <string>
#include "track.h"
#include <iostream>

enum AnimationType {
    ANIM_TYPE_UNDEFINED,
    ANIM_TYPE_SIMPLE,
    ANIM_TYPE_SKELETON,
};

class Animation {

    std::vector<Track> tracks;
    std::string name;

    float start_time = 0.0f;
    float end_time = 0.0f;
    bool looping = false;

    float adjust_time_to_fit_range(float time);

public:

    Animation();
    ~Animation();

    // Gets joint Id based for a specific track index
    uint32_t get_id_at_index(uint32_t index);
    // Sets joint ID based on the index of the joint in the clip
    void set_id_at_index(uint32_t idx, uint32_t id);
    uint32_t size();

    // Samples the animation clip at the provided time into the out reference
    float sample(float time);

    // Returns a transform track for the specified joint
    Track* operator[](uint32_t index);

    // Sets the start/end time of the animation clip based on the tracks that make up the clip
    void recalculate_duration();

    // Adds a new track with the pointer property data of any node
    Track* add_track(uint32_t id, void* data);

    Track* get_track(uint32_t index);
    std::string& get_name();
    float get_duration();
    float get_start_time();
    float get_end_time();
    bool get_looping();

    void set_name(const std::string& in_new_name);
    void set_looping(bool in_looping);
};


