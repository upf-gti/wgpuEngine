#include "animation_mixer.h"

AnimationMixer::AnimationMixer() {

}

AnimationMixer::AnimationMixer(Node3D* node) {
    root_node = node;
}

void AnimationMixer::set_root_node(Node3D* node) {
    root_node = node;
}

RendererStorage::AnimationData* AnimationMixer::get_animation(std::string animation_name) {
    return RendererStorage::get_animation(animation_name);
}
