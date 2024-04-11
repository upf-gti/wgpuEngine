#pragma once
#include <vector>
#include <string>
#include "track.h"

class Animation {
protected:

    std::vector<ScalarTrack> tracks;
    std::string name;
    float start_time;
    float end_time;
    bool looping;

protected:
    float adjust_time_to_fit_range(float inTime);

public:
    Animation();
    ~Animation();
    //gets joint Id based for a specific track index
    unsigned int get_id_at_index(unsigned int index);
    //sets joint ID based on the index of the joint in the clip
    void set_id_at_index(unsigned int idx, unsigned int id);
    unsigned int size();

    //samples the animation clip at the provided time into the out reference
    float sample(std::vector<float> &out, float inTime);
    //returns a transform track for the specified joint
    ScalarTrack& operator[](unsigned int index);
    ScalarTrack& get_track(unsigned int index);

    //sets the start/end time of the animation clip based on the tracks that make up the clip
    void recalculate_duration();

    std::string& get_name();
    void set_name(const std::string& in_new_name);
    float get_duration();
    float get_start_time();
    float get_end_time();
    bool get_looping();
    void set_looping(bool in_looping);
};


