#pragma once

#include "track.h"

#include "framework/resources/resource.h"

#include <iostream>

enum eAnimationType {
    ANIMATION_TYPE_UNDEFINED,
    ANIMATION_TYPE_SIMPLE,
    ANIMATION_TYPE_SKELETON,
};

enum eBlendType : uint8_t {
    ANIMATION_BLEND_CROSSFADE,
    ANIMATION_BLEND_ADDITIVE
};

class Animation : public Resource {

    static uint32_t last_animation_id;

    std::vector<Track> tracks;
    std::string name;

    float start_time = 0.0f;
    float end_time = 0.0f;
    float duration = 0.0f;

    bool reversed = false;

    float adjust_time_to_fit_range(float time, uint8_t loop);

    eAnimationType type = ANIMATION_TYPE_UNDEFINED;

public:

    Animation();
    ~Animation() {};

    // Gets joint Id based for a specific track index
    int get_id_at_index(uint32_t index);

    // Sets joint ID based on the index of the joint in the clip
    void set_id_at_index(uint32_t index, int id);

    // Samples the animation clip at the provided time into the out reference
    float sample(float time, uint32_t track_idx, uint8_t loop, Node::AnimatableProperty* out = nullptr, eInterpolationType interpolation_type = INTERPOLATION_UNSET);

    // Returns a transform track for the specified track position id
    Track* operator[](uint32_t index);

    // Sets the start/end time of the animation clip based on the tracks that make up the clip
    void recalculate_duration();

    // Adds a new track
    Track* add_track(int id = -1);

    uint32_t get_track_count();
    Track* get_track_by_id(int id);
    Track* get_track(uint32_t index);
    std::string& get_name();
    float get_duration();
    float get_start_time();
    float get_end_time();
    eAnimationType get_type();
    bool is_reversed();

    void serialize(std::ofstream& binary_scene_file);
    void parse(std::ifstream& binary_scene_file);

    void set_type(eAnimationType new_type);
    void set_name(const std::string& new_name);
};
