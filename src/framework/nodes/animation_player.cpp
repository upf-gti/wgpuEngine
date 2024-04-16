#include "animation_player.h"

#include "graphics/renderer_storage.h"
#include "framework/input.h"
#include "imgui.h"
#include "spdlog/spdlog.h"

#include "skeleton_instance_3d.h"

AnimationPlayer::AnimationPlayer(const std::string& n)
{
    name = n;
}

void AnimationPlayer::play(const std::string& animation_name, float custom_blend, float custom_speed, bool from_end)
{
    if (!root_node) {
        root_node = get_parent();
    }

    Animation* animation = RendererStorage::get_animation(animation_name);
    if (!animation) {
        spdlog::error("No animation called {}", animation_name);
        return;
    }

    if (custom_blend >= 0.0f) {
        blend_time = custom_blend;
        blender.fade_to(animation, blend_time);
    }
    else {
        blender.play(animation);
        playback = 0.0f;
    }

    speed = custom_speed;
    playing = true;
    current_animation_name = animation_name;

    generate_track_data();
}

void AnimationPlayer::generate_track_data()
{
    track_data.clear();

    Animation* animation = RendererStorage::get_animation(current_animation_name);
    uint32_t num_tracks = animation->get_track_count();
    track_data.resize(num_tracks);

    // Generate data from tracks

    for (uint32_t i = 0; i < num_tracks; ++i) {

        const std::string& track_path = animation->get_track(i)->get_path();

        size_t last_idx = track_path.find_last_of('/');
        const std::string& node_path = track_path.substr(0, last_idx);
        const std::string& property_name = track_path.substr(last_idx + 1);

        // this get_node is overrided by SkeletonInstance, depending on the class
        // it will use hierarchical search or linear search in joints array
        Node* node = root_node->get_node(node_path);

        track_data[i] = node->get_property(property_name);
    }
}

void AnimationPlayer::pause()
{
    playing = !playing;
}

void AnimationPlayer::stop(bool keep_state)
{
    blender.stop();
    playing = false;
}

void AnimationPlayer::update(float delta_time)
{
    if (!playing) {
        return;
    }

    Animation* current_animation = blender.get_current_animation();
    assert(current_animation);

    if (!current_animation->get_looping() && playback >= current_animation->get_duration()) {
        playing = false;
    }

    // Sample data from the animation and store it at &track_data

    playback += delta_time * speed;

    for (uint32_t i = 0; i < current_animation->get_track_count(); ++i) {
        playback = current_animation->sample(playback, i, track_data[i]);
    }

    // After sampling, we should have the skeletonInstance joint nodes with the correct
    // transforms.. so use those nodes to update the pose of the skeletons

    SkeletonInstance3D* sk_instance = (SkeletonInstance3D*)root_node->get_children()[0];
    sk_instance->update_pose_from_joints();

    //for (auto instance : root_node->get_children()) {

    //    MeshInstance3D* node = dynamic_cast<MeshInstance3D*>(instance);

    //    if (!node) {
    //        continue;
    //    }

    //    // skeletal animation case: we get the skeleton pose
    //    // in case we want to process the values and update it manually
    //    if (current_animation->get_type() == anim_type_skeleton) {
    //        skeleton* skeleton = node->get_skeleton();
    //        if (!skeleton) {
    //            continue;
    //        }
    //        pose& pose = skeleton->get_current_pose();
    //        playback = blender.update(delta_time * speed, &pose);
    //    }
    //    else {
    //        // general case: out is not used..
    //        playback = blender.update(delta_time * speed);
    //    }
    //}

    Node::update(delta_time);
}

void AnimationPlayer::render_gui()
{
    if (ImGui::BeginCombo("##current", current_animation_name.c_str()))
    {
        for (auto& instance : RendererStorage::animations)
        {
            bool is_selected = (current_animation_name == instance.first);
            if (ImGui::Selectable(instance.first.c_str(), is_selected)) {
                play(instance.first, blend_time);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Checkbox("Loop", &looping)) {
        Animation* current_animation = blender.get_current_animation();
        if (current_animation) {
            current_animation->set_looping(looping);
        }
    }

    ImGui::DragFloat("Blend time", &blend_time, 0.1f);
    ImGui::LabelText(std::to_string(playback).c_str(), "Playback");

    if (!playing && ImGui::Button("Play")) {
        play(current_animation_name);
    }
    else if(playing && ImGui::Button("Stop")) {
        stop();
    }

    if (ImGui::Button("Pause")) {
        pause();
    }
}

void AnimationPlayer::set_speed(float time)
{
    speed = time;
}

void AnimationPlayer::set_blend_time(float time)
{
    blend_time = time;
}

void AnimationPlayer::set_looping(bool loop)
{
    looping = loop;

    Animation* current_animation = blender.get_current_animation();

    if (current_animation)
    {
        current_animation->set_looping(looping);
    }
}

void AnimationPlayer::set_root_node(Node3D* new_root_node)
{
    this->root_node = new_root_node;
}

float AnimationPlayer::get_blend_time()
{
    return blend_time;
}

float AnimationPlayer::get_speed()
{
    return speed;
}

bool AnimationPlayer::is_looping()
{
    return looping;
}

bool AnimationPlayer::is_playing()
{
    return playing;
}
