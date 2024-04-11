#pragma once

#include "animation_mixer.h"
#include "skeleton_instance_3d.h"
#include "../animation/skeletal_animation.h"

class AnimationPlayer : public AnimationMixer, public Node3D
{
    std::string current_animation;
    std::string next_animation;
    bool autoplay = false;
    bool playing = false;
    float playback = 0.0;
    float duration;
    float speed = 1.0;
    float blend_time;
    float looping = false;

    std::string type = "simple";

    std::vector<std::string> animations_queue;

    Animation* animation = nullptr;
    Node3D* node = nullptr;

public:

    void set_next_animation(const std::string animation_name);
    void set_speed(float time);
    void set_blend_time(float time);
    void set_looping(bool loop);

    std::string get_current_animation();
    std::string get_next_animation();
    float get_blend_time();
    float get_speed();
    std::vector<std::string> get_queue();

    void queue(std::string animation_name);
    void clear_queue();

    bool is_looping();
    bool is_playing();
    void play(std::string animation_name = "", float custom_blend = -1, float custom_speed = 1.0, bool from_end = false);
    void pause();
    void stop(bool keep_state = false);

    void update(float delta_time);
};

