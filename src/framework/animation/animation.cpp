#include "animation.h"

Animation::Animation() {
    name = "No name given";
    start_time = 0.0f;
    end_time = 0.0f;
    looping = true;
}

float Animation::sample(Pose& outPose, float time) {
    if (get_duration() == 0.0f) {
        return 0.0f;
    }
    time = adjust_time_to_fit_range(time);
    unsigned int size = tracks.size();
    for (unsigned int i = 0; i < size; ++i) {
        // get the joint ID of the  track
        unsigned int j = tracks[i].get_id(); // Joint
        Transform local = outPose.get_local_transform(j);
        // sample the track
        Transform animated = tracks[i].sample(local, time, looping);
        // assign the sampled value back to the Pose reference
        outPose.set_local_transform(j, animated);
    }
    return time;
}

float Animation::adjust_time_to_fit_range(float inTime) {
    if (looping) {
        float duration = end_time - start_time;
        if (duration <= 0) { 0.0f; }
        inTime = fmodf(inTime - start_time, end_time - start_time);
        if (inTime < 0.0f) {
            inTime += end_time - start_time;
        }
        inTime = inTime + start_time;
    }
    else {
        if (inTime < start_time) {
            inTime = start_time;
        }
        if (inTime > end_time) {
            inTime = end_time;
        }
    }
    return inTime;
}

void Animation::recalculate_duration() {
    start_time = 0.0f;
    end_time = 0.0f;
    bool startSet = false;
    bool endSet = false;
    unsigned int tracksSize = tracks.size();
    for (unsigned int i = 0; i < tracksSize; ++i) {
        if (tracks[i].is_valid()) {
            float minstart_time = tracks[i].get_start_time();
            float max_end_time = tracks[i].get_end_time();
            if (minstart_time < start_time || !startSet) {
                start_time = minstart_time;
                startSet = true;
            }
            if (max_end_time > end_time || !endSet) {
                end_time = max_end_time;
                endSet = true;
            }
        }
    }
}

// retrieves the TransformTrack object for a specific joint in the Animation
TransformTrack& Animation::operator[](unsigned int joint) {
    return get_track(joint);
}

// retrieves the TransformTrack object for a specific joint in the Animation
TransformTrack& Animation::get_track(unsigned int joint) {
    for (int i = 0, s = tracks.size(); i < s; ++i) {
        if (tracks[i].get_id() == joint) {
            return tracks[i];
        }
    }
    // if no qualifying track is found, a new one is created and returned
    tracks.push_back(TransformTrack());
    tracks[tracks.size() - 1].set_id(joint);
    return tracks[tracks.size() - 1];
}

// getters
std::string& Animation::get_name() {
    return name;
}

unsigned int Animation::get_id_at_index(unsigned int index) {
    return tracks[index].get_id();
}

unsigned int Animation::size() {
    return (unsigned int)tracks.size();
}

float Animation::get_duration() {
    return end_time - start_time;
}

float Animation::get_start_time() {
    return start_time;
}

float Animation::get_end_time() {
    return end_time;
}

bool Animation::get_looping() {
    return looping;
}

// setters
void Animation::set_name(const std::string& inNewName) {
    name = inNewName;
}

void Animation::set_id_at_index(unsigned int index, unsigned int id) {
    return tracks[index].set_id(id);
}

void Animation::set_looping(bool inLooping) {
    looping = inLooping;
}
