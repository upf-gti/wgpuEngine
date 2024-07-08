#pragma once

#include "node_3d.h"
#include "framework/animation/blend_animation.h"

#include "framework/ui/timeline.h"
#include "framework/animation/time_tunnel.h"

class MeshInstance3D;

enum BlendType {
    CROSSFADE,
    ADDITIVE
};

struct Trajectory
{
    std::string node_name;
    uint32_t track_id;
    MeshInstance3D* mesh;
    std::vector<glm::vec3> trajectory;
};

class AnimationPlayer : public Node3D
{
    Node3D* root_node = nullptr;

    std::string current_animation_name;
    Animation* current_animation = nullptr;

    bool autoplay   = false;
    bool playing    = false;
    bool looping    = true;
    bool blend_type = BlendType::CROSSFADE;

    float playback   = 0.f;
    float speed      = 1.f;
    float blend_time = 0.f;
    uint32_t current_frame = 0;

    BlendAnimation blender;
    Timeline timeline;
    TimeTunnel time_tunnel;
    std::vector<Pose> keyposes;
    std::vector<uint32_t> active_tracks;

    std::vector<void*> track_data;
    int selected_track = -1;
    std::vector<MeshInstance3D*> keyposes_helper;
    std::vector<MeshInstance3D*> trajectories_helper;
    std::vector<Trajectory> trajectories_helper2;
    std::vector<MeshInstance3D*> smoothed_trajectories_helper;
    void generate_track_data();
    void generate_track_timeline_data(uint32_t track_idx, const std::string& track_path);
    void generate_keyposes();
    void update_trajectories(std::vector<uint32_t>& tracks);

    void compute_keyframes();

public:

    AnimationPlayer(const std::string& name);

    void play(const std::string& animation_name = "", float custom_blend = -1, float custom_speed = 1.0, bool from_end = false);
    void pause();
    void stop(bool keep_state = false);

    void update(float delta_time) override;
    void render_gui() override;
    void render() override;

    void set_speed(float time);
    void set_blend_time(float time);
    void set_looping(bool loop);
    void set_root_node(Node3D* new_root_node);

    float get_blend_time();
    float get_speed();
    bool is_playing();
    bool is_looping();
};

