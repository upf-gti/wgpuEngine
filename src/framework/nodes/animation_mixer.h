#pragma once

#include "node_3d.h"
#include "framework/animation/animation.h"
#include "../graphics/renderer_storage.h"

class AnimationMixer : public Node3D
{
protected:
    Node3D* root_node;
    bool active = true;

public:
    AnimationMixer();
    AnimationMixer(Node3D* root_node);

    void set_root_node(Node3D* node);
    AnimationData* get_animation(std::string animation_name);
};

