#pragma once

#include "animation.h"

struct BlendTarget {
    Animation* animation = nullptr;
    float time = 0.0f;
    float duration = 0.0f;
    float elapsed = 0.0f;
    inline BlendTarget() : animation(0), time(0.0f), duration(0.0f), elapsed(0.0f) { }
    inline BlendTarget(Animation* target, float duration) : animation(target), time(target->get_start_time()), duration(duration), elapsed(0.0f) { }
};

// Fast blend from one animation to another (hide the transition between two animations)
class BlendAnimation {

protected:
    std::vector<BlendTarget> targets;
    Animation* animation = nullptr;
    float time = 0.0f;

public:
    BlendAnimation();

    void play(Animation* target);
    void fade_to(Animation* target, float fade_time);
    void update(float dt);

    void blend(T& out, T& a, T& b, float time);

    Animation* get_current_animation();
};
