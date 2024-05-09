#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <map>

#include "track.h"

enum AnimationType {
    ANIM_TYPE_UNDEFINED,
    ANIM_TYPE_SIMPLE,
    ANIM_TYPE_SKELETON,
};

class Animation;

class Animation {

    static uint32_t last_animation_id;

    std::vector<Track> tracks;
    std::vector<uint32_t> keyposes;
    std::string name;

    float start_time = 0.0f;
    float end_time = 0.0f;
    bool looping = true;
    float duration = 0.0f;

    float adjust_time_to_fit_range(float time);
    void sample_pose(float time, void* out);
    float compute_difference(uint32_t joint_idx, float t, std::map<std::string, glm::vec3>& joint_tracks);

    AnimationType type = ANIM_TYPE_UNDEFINED;

public:

    Animation();
    ~Animation() {};

    // Gets joint Id based for a specific track index
    int get_id_at_index(uint32_t index);

    // Sets joint ID based on the index of the joint in the clip
    void set_id_at_index(uint32_t index, int id);

    // Samples the animation clip at the provided time into the out reference
    float sample(float time, uint32_t index, void* out = nullptr);

    // Returns a transform track for the specified track position id
    Track* operator[](uint32_t index);

    // Sets the start/end time of the animation clip based on the tracks that make up the clip
    void recalculate_duration();

    // Adds a new track
    Track* add_track(int id = -1);

    // Compute Joint Trajectory in a specific time given its position track, the direction of the tunnel and a degree of stretchiness
    std::vector<glm::vec3> compute_trajectory(Track& track, const glm::vec3& axis, float stretchiness, float t);
    std::map < std::string, std::vector<glm::vec3>> compute_smoothed_trajectory(std::map<std::string, std::vector<glm::vec3>>& trajectories, std::map<std::string, std::vector<glm::vec3>>& smoothed_trajectories, float window, float current_t);

    // Extract KeyPoses
    void compute_keyposes();
    const std::vector<uint32_t>& get_keyposes();

    uint32_t get_track_count();
    Track* get_track_by_id(int id);
    Track* get_track_by_path(const std::string& path);
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

    void remove_frame(uint32_t index);
};


