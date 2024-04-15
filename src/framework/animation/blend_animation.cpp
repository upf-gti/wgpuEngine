#include "blend_animation.h"

BlendAnimation::BlendAnimation()
{
    animation = nullptr;
    time = 0.0f;
}

/*
* Plays the controller:
* Clear any active crossfades.
* It sets the animation and playback time, and reset the current pose to the rest pose of the skeleton
*/
void BlendAnimation::play(Animation* target)
{
    targets.clear();
    animation = target;
    time = target->get_start_time();
}

/*
* Adds the provided animation clip and duration to the fade list
* A fade target is only valid if it is not the first or last item in the fade list
*/
void BlendAnimation::fade_to(Animation* target, float fade_time)
{
    if (!animation) {
        play(target);
        return;
    }

    if (targets.size() >= 1) {
        Animation* clip_to_blend = targets[targets.size() - 1].animation;
        if (clip_to_blend == target) {
            return;
        }
    }
    else if (animation == target) {
        return;
    }

    targets.push_back(BlendTarget(target, fade_time));
}

// Play the active animation and blend in any other animations that are in the fade list
void BlendAnimation::update(float dt)
{
    if (!animation) {
        return;
    }

    // Set the current animation as the target animation and remove the fade object if an animation has finished fading
    unsigned int num_targets = targets.size();
    for (unsigned int i = 0; i < num_targets; ++i) {
        float duration = targets[i].duration;
        if (targets[i].elapsed >= duration) {
            animation = targets[i].animation;
            time = targets[i].time;
            targets.erase(targets.begin() + i);
            break;
        }
    }
    // Blend the fade list with the current animation
    num_targets = targets.size();

    // time = animation->sample(time + dt);

    //for (unsigned int i = 0; i < num_targets; ++i) {
    //    BlendTarget& target = targets[i];
    //    target.time = target.animation->sample(target.time + dt);
    //    target.elapsed += dt;
    //    float t = target.elapsed / target.duration;
    //    if (t > 1.0f) {
    //        t = 1.0f;
    //    }
    //    //blend(pose, pose, target.pose, t, -1);
    //}
}

Animation* BlendAnimation::get_current_animation()
{
    return animation;
}

void BlendAnimation::blend(T& out, T& a, T& b, float time)
{
    if (animation->get_duration() == 0.0f) {
        return;
    }

    size_t size = animation->size();

    for (size_t i = 0; i < size; ++i) {

        Track* track = animation->get_track(i);
        if (std::holds_alternative<float>(track->operator[](0).value)) {

        }
        // sample the track
        // track->sample(time, animation->get_looping(), track->get_data());
    }

    //time = animation->adjust_time_to_fit_range(time);
}
