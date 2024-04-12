#include "animation_player.h"

void AnimationPlayer::set_next_animation(const std::string animation_name)
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

void AnimationPlayer::queue(std::string animation_name)
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

void AnimationPlayer::play(std::string animation_name, float custom_blend, float custom_speed, bool from_end)
{
    if (animation_name != "") {
        current_animation = animation_name;
    }
    else {
        current_animation = animations_queue[0];
        animations_queue.erase(animations_queue.begin());
    }
    if (custom_blend >= 0) {
        blend_time = custom_blend;
    }
    speed = custom_speed;
    RendererStorage::AnimationData* data = get_animation(current_animation);
    animation = data->animation;
    type = data->animation_type;

    for (auto const& instance : root_node->get_children()) {
        if (instance->get_name() == data->node_path) {
            node = (Node3D*)instance;
            break;
        }
    }
    animation->set_looping(looping);
    duration = animation->get_duration();
    playing = true;
    playback = 0;
}

void AnimationPlayer::pause()
{
}

void AnimationPlayer::stop(bool keep_state)
{
}


void AnimationPlayer::update(float delta_time) {

    if (playback >= duration) { // TO DO: check if works when looping is true
        playing = false;
    }

    if (playing) {
        if (type == "skeleton") {
          /*  Skeleton* skeleton = (static_cast<SkeletonInstance3D*>(node))->get_skeleton();
            playback = (static_cast<SkeletalAnimation*>(animation))->sample(skeleton->get_current_pose(), delta_time * speed);*/

        }
    }
    Node::update(delta_time);
}
