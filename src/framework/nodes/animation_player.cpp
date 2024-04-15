#include "animation_player.h"

#include "framework/input.h"
#include "imgui.h"

AnimationPlayer::AnimationPlayer(const std::string& n) {
    name = n;
}

void AnimationPlayer::set_next_animation(const std::string& animation_name)
{
    animations_queue.push_back(animation_name);
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

    // debug
    animation->set_looping(true);
}

void AnimationPlayer::pause()
{

}

void AnimationPlayer::stop(bool keep_state)
{

}

void AnimationPlayer::update(float delta_time)
{
    if (!looping && playback >= duration) {
        playing = false;
    }

    if (playing) {

        playback += delta_time * speed;
        playback = animation->sample(playback);
    }

    Node::update(delta_time);
}

void AnimationPlayer::render_gui()
{
    ImGui::Checkbox("Play", &playing);
    ImGui::Checkbox("Loop", &looping);
}
