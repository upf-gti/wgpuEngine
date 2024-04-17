#pragma once

#include "node_3d.h"
#include "../animation/blend_animation.h"

class MeshInstance3D;

enum BlendType {
    CROSSFADE,
    ADDITIVE
};

class AnimationPlayer : public Node3D
{
    Node3D* root_node = nullptr;

    std::string current_animation_name;

    bool active     = true;
    bool autoplay   = false;
    bool playing    = false;
    bool looping    = true;
    bool blend_type = BlendType::CROSSFADE;

    float playback   = 0.f;
    float speed      = 1.f;
    float blend_time = 0.f;

    BlendAnimation blender;

    std::vector<void*> track_data;

    void generate_track_data();

public:

    AnimationPlayer(const std::string& name);

    void play(const std::string& animation_name = "", float custom_blend = -1, float custom_speed = 1.0, bool from_end = false);
    void pause();
    void stop(bool keep_state = false);

    void update(float delta_time);
    void render_gui() override;

    void set_speed(float time);
    void set_blend_time(float time);
    void set_looping(bool loop);
    void set_root_node(Node3D* new_root_node);

    float get_blend_time();
    float get_speed();
    bool is_playing();
    bool is_looping();
};

