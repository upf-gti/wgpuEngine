#include "animation_player.h"

#include "framework/input.h"
#include "imgui.h"

AnimationPlayer::AnimationPlayer(const std::string& n) {
    name = n;
}

void AnimationPlayer::set_next_animation(const std::string& animation_name)
{
    animations_queue.push_back(animation_name);
    AnimationData* data = get_animation(animation_name);
    animation = data->animation;
    current_animation = animation_name;
    blender.fade_to(animation, blend_time);
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
    if (animation)
    {
        animation->set_looping(looping);
    }
}

std::string AnimationPlayer::get_current_animation()
{
    return current_animation;
}

std::string AnimationPlayer::get_next_animation()
{
    if (!animations_queue.size())
        return "";

    return animations_queue[0];
}

float AnimationPlayer::get_blend_time()
{
    return blend_time;
}

float AnimationPlayer::get_speed()
{
    return speed;
}

std::vector<std::string> AnimationPlayer::get_queue()
{
    return animations_queue;
}

void AnimationPlayer::queue(const std::string& animation_name)
{
    animations_queue.push_back(animation_name);
}

void AnimationPlayer::clear_queue()
{
    animations_queue.clear();
    animations_queue.resize(0);
}

bool AnimationPlayer::is_looping()
{
    return looping;
}

bool AnimationPlayer::is_playing()
{
    return playing;
}

void AnimationPlayer::play(const std::string& animation_name, float custom_blend, float custom_speed, bool from_end)
{
    if (animation_name != "") {
        current_animation = animation_name;
    }
    else {
        current_animation = animations_queue[0];
        animations_queue.erase(animations_queue.begin());
    }
    if (custom_blend >= 0.0f) {
        blend_time = custom_blend;
    }

    AnimationData* data = get_animation(current_animation);
    animation = data->animation;

    animation->set_looping(looping);
    duration = animation->get_duration();
    speed = custom_speed;
    playing = true;
    playback = 0.0f;

    for (auto instance : root_node->get_children()) {
        if (instance->get_name() == data->node_path) {
            node = static_cast<MeshInstance3D*>(instance);
            break;
        }
    }
    blender.play(animation);
}

void AnimationPlayer::pause()
{

}

void AnimationPlayer::stop(bool keep_state)
{
    blender.stop();
    playing = false;
}

void AnimationPlayer::update(float delta_time)
{
    if (playback >= duration) {
        playing = false;
    }

    if (playing) {

       // playback += delta_time * speed;

        // Skeletal animation case: we get the skeleton pose
        // in case we want to process the values and update it manually
        if (animation->get_type() == ANIM_TYPE_SKELETON) {

            auto skt = node->get_skeleton();
            Pose pose = skt->get_current_pose();
            //playback = animation->sample(playback, &pose);
            playback = blender.update(delta_time * speed, &pose);

            // Blend animations and update pose manually
            {
                skt->set_current_pose(pose);
            }
        }
        else {
            // General case: out is not used..
            //playback = animation->sample(playback);
            playback = blender.update(delta_time * speed);
        }
    }

    Node::update(delta_time);
}

void AnimationPlayer::render_gui()
{     
    if (ImGui::BeginCombo("##current", current_animation.c_str())) // The second parameter is the label previewed before opening the combo.
    {
        for (auto& instance : RendererStorage::animations)
        {
            bool is_selected = (current_animation == instance.first); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable(instance.first.c_str(), is_selected)) {
                set_next_animation(instance.first);
               // play();
            }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }
        ImGui::EndCombo();
    }

    if (ImGui::Checkbox("Loop", &looping)) {
        animation->set_looping(looping);
    }

    ImGui::DragFloat("Blend time", &blend_time, 0.1f);
    ImGui::LabelText(std::to_string(playback).c_str(), "Playback");

    if (!playing && ImGui::Button("Play")) {
        play(current_animation);
    }
    else if(playing && ImGui::Button("Stop")) {
        stop();
    }
}
