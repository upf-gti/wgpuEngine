#pragma once

#include "node_3d.h"
#include "framework/animation/blend_animation.h"

#include "framework/ui/imgui_timeline.h"

class MeshInstance3D;

class AnimationPlayer : public Node3D
{
    std::string current_animation_name;

    bool autoplay   = false;
    bool playing    = false;
    bool paused     = false;

    float playback   = 0.f;
    float speed      = 1.f;
    float blend_time = 0.f;

    uint8_t blend_type = ANIMATION_BLEND_CROSSFADE;
    uint8_t loop_type = ANIMATION_LOOP_DEFAULT;

    BlendAnimation blender;
    ImGuiTimeline imgui_timeline;

    std::vector<AnimatableProperty> track_data;
    int selected_track = -1;

    void generate_track_data();

#ifdef __EMSCRIPTEN__
public:
#endif
    Node3D* root_node = nullptr;

public:

    AnimationPlayer();
    AnimationPlayer(const std::string& name);

    void play(Animation* animation = nullptr, float start_time = 0.0f, float custom_blend = -1.0f, float custom_speed = 1.0f);
    void play(const std::string& animation_name = "", float start_time = 0.0f, float custom_blend = -1.0f, float custom_speed = 1.0f);
    void pause();
    void resume();
    void stop(bool keep_state = false);

    void update(float delta_time) override;
    void render_gui() override;

    void set_playback_time(float time);
    void set_speed(float new_speed) { speed = new_speed; }
    void set_blend_time(float new_blend_time) { blend_time = new_blend_time; }
    void set_loop_type(uint8_t new_loop_type) { loop_type = new_loop_type; }
    void set_root_node(Node3D* new_root_node) { root_node = new_root_node; }

    float get_playback_time() const { return playback; }
    float get_blend_time() const { return blend_time; }
    float get_speed() const { return speed; }
    uint8_t get_loop_type() const { return loop_type; }
    Node3D* get_root_node() const { return root_node; }
    bool is_playing() const { return playing; }
    bool is_paused() const { return paused; }
};

