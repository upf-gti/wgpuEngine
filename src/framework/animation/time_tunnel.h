#pragma once

#include <vector>

#include "track.h"

class TimeTunnel
{
    std::vector<std::vector<glm::vec3>> trajectories;
    std::vector<uint32_t> keyframes;
    std::vector<std::vector<float>> magnitudes;
    std::vector<float> total_magnitudes;

    glm::vec3 axis = glm::vec3(1.0f, 0.0f, 0.0f);
    float stretchiness = 5.0f;
    uint32_t current_frame;
    uint32_t number_frames = 10;
    float threshold = 0.00001f;
    float gaussian_sigma = 1.0f;

    glm::vec3 compute_smoothed_trajectory_position(float t, float max_diff_t, std::vector<glm::vec3>& trajectory, std::vector<glm::vec3>& smoothed_trajectory);
    std::vector<glm::vec3> compute_gaussian_smoothed_trajectory(std::vector<glm::vec3>& trajectory);

public:

    void set_axis(const glm::vec3& axis_direction);
    void set_stretchiness(const float stretchiness_window);
    void set_current_frame(const uint32_t tc);
    void set_number_frames(const uint32_t count);
    void set_threshold(const float threshold_diff);
    void set_gaussian_sigma(const float sigma);

    glm::vec3& get_axis();
    float get_stretchiness();
    uint32_t get_current_frame();
    uint32_t get_number_frames();
    float get_threshold();
    float get_gaussian_sigma();
    std::vector<uint32_t> get_keyframes();
    std::vector<std::vector<glm::vec3>> get_trajectories();

    glm::vec3 compute_trajectory_position(float t, const glm::vec3& position);
    std::vector<std::vector<glm::vec3>> compute_trajectories(std::vector<Track*>& tracks);

    std::vector<uint32_t> extract_keyframes(std::vector<Track*>& tracks);
};


