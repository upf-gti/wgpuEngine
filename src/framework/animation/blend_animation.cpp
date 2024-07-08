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

void BlendAnimation::stop()
{
    animation = nullptr;
    time = 0.0f;
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
float BlendAnimation::update(float current_time, std::vector<void*>& data)
{
    if (!animation) {
        return time;
    }

    // Set the current animation as the target animation and remove the fade object if an animation has finished fading
    size_t num_targets = targets.size();
    for (size_t i = 0; i < num_targets; ++i) {
        float duration = targets[i].duration;
        if (targets[i].elapsed >= duration) {
            animation = targets[i].animation;
            time = targets[i].time;
            targets.erase(targets.begin() + i);
            break;
        }
    }

    for (uint32_t i = 0; i < animation->get_track_count(); ++i) {
        time = animation->sample(current_time, i, data[i]);
    }

    // Blend the fade list with the current animation

    num_targets = targets.size();

    if (animation->get_type() == ANIM_TYPE_SKELETON) {

        //Pose* pose = (Pose*)(&data);
        Pose* pose = (Pose*)(&data);
        Pose* target_pose = new Pose(pose->size());


        //for (uint32_t i = 0; i < num_targets; ++i) {
        //    BlendTarget& target = targets[i];
        //    std::vector<void*> d(target.animation->get_track_count());
        //    for (uint32_t j = 0; j < target.animation->get_track_count(); ++j) {
        //       
        //        target.time = target.animation->sample(target.time + dt, j, d[j]);
        //    }
        //    target.elapsed += dt;
        //    float t = target.elapsed / target.duration;
        //    if (t > 1.0f) {
        //        t = 1.0f;
        //    }
        //    Pose* target_pose = (Pose*)(&d);
        //    //if(blend_type)
        //    blend(*pose, *pose, *target_pose, t);
        //}
    }

    return time;
}

Animation* BlendAnimation::get_current_animation()
{
    return animation;
}

void BlendAnimation::blend(Pose& output, Pose& a, Pose& b, float t)
{
    size_t num_joints = output.size();
    for (size_t i = 0; i < num_joints; ++i) {        
        output.set_local_transform(i, Transform::mix(a.get_local_transform(i), b.get_local_transform(i), t));
    }
}

void BlendAnimation::add(Pose& output, Pose& in, Pose& add_pose, Pose& base_pose)
{
    size_t numJoints = add_pose.size();
    for (size_t i = 0; i < numJoints; ++i) {
        Transform input = in.get_local_transform(i);
        Transform additive = add_pose.get_local_transform(i);
        Transform additiveBase = base_pose.get_local_transform(i);
   
        // outPose = inPose + (addPose - basePose)
        Transform result(
            input.get_position() + (additive.get_position() - additiveBase.get_position()),
            normalize(input.get_rotation() * (inverse(additiveBase.get_rotation()) * additive.get_rotation())),
            input.get_scale() + (additive.get_scale() - additiveBase.get_scale()));
        output.set_local_transform(i, result);
    }
}
