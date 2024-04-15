#pragma once

#include "animation.h"

struct BlendTarget {
    Animation* animation;
    float time;
    float duration;
    float elapsed;
    inline BlendTarget() : animation(0), time(0.0f), duration(0.0f), elapsed(0.0f) { }
    inline BlendTarget(Animation* target,float dur) : animation(target), time(target->get_start_time()), duration(dur), elapsed(0.0f) { }
};

// Fast blend from one animation to another (hide the transition between two animations)
class BlendAnimation {
protected:
    std::vector<BlendTarget> targets;
    Animation* animation;
    float time;
public:
    BlendAnimation();
    void play(Animation* target);
    void fadeTo(Animation* target, float fadeTime);
    void update(float dt);
    Animation* get_current_animation();
    void blend(T& out, T& a, T& b, float time);
};
