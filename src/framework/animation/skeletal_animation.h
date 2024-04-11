#pragma once
#include "animation.h"
#include "transform_track.h"
#include "pose.h"

class SkeletalAnimation : public Animation
{
    std::vector<TransformTrack> tracks;

public:
    //samples the animation clip at the provided time into the Pose reference
    float sample(Pose& outPose, float inTime);
    //returns a transform track for the specified joint
    TransformTrack& operator[](unsigned int index);
    TransformTrack& get_track(unsigned int index);
    void recalculate_duration();
};

