#pragma once

#include "animation_mixer.h"
#include "skeleton_instance_3d.h"

class AnimationPlayer : public AnimationMixer
{
    std::string current_animation;
    std::string next_animation;

    bool autoplay   = false;
    bool playing    = false;
    bool looping    = true;

    float playback   = 0.f;
    float duration   = 0.f;
    float speed      = 1.f;
    float blend_time = 0.f;

    AnimationType type = AnimationType::ANIM_TYPE_SIMPLE;

    std::vector<std::string> animations_queue;

    Animation* animation = nullptr;
    MeshInstance3D* node = nullptr;

public:
    AnimationPlayer(const std::string& name);
    void set_next_animation(const std::string& animation_name);
    void set_speed(float time);
    void set_blend_time(float time);
    void set_looping(bool loop);

    std::string get_current_animation();
    std::string get_next_animation();
    float get_blend_time();
    float get_speed();
    std::vector<std::string> get_queue();

    void queue(const std::string& animation_name);
    void clear_queue();

    bool is_looping();
    bool is_playing();
    void play(const std::string& animation_name = "", float custom_blend = -1, float custom_speed = 1.0, bool from_end = false);
    void pause();
    void stop(bool keep_state = false);

    void update(float delta_time);
    void render_gui() override;
};

