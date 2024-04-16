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
    if (playing) {

        Animation* current_animation = blender.get_current_animation();
        assert(current_animation);

        if (!current_animation->get_looping() && playback >= current_animation->get_duration()) {
            playing = false;
        }

        for (auto instance : root_node->get_children()) {

            MeshInstance3D* node = dynamic_cast<MeshInstance3D*>(instance);

            if (!node) {
                continue;
            }

            Skeleton* skeleton = node->get_skeleton();
            if (!skeleton) {
                continue;
            }

            // Skeletal animation case: we get the skeleton pose
            // in case we want to process the values and update it manually
            if (current_animation->get_type() == ANIM_TYPE_SKELETON) {

                Pose& pose = skeleton->get_current_pose();
                playback = blender.update(delta_time * speed, &pose);
            }
            else {
                // General case: out is not used..
                playback = blender.update(delta_time * speed);
            }
        }
    }

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
