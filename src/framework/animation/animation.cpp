#include "animation.h"

uint32_t Animation::last_animation_id = 0;

Animation::Animation()
{
    name = "AnimationUnnamed_" + std::to_string(last_animation_id++);
    start_time = 0.0f;
    end_time = 0.0f;
    looping = true;
}

float Animation::sample(float time, uint32_t track_idx, void* out)
{
    if (get_duration() == 0.0f) {
        return 0.0f;
    }

    time = adjust_time_to_fit_range(time);

    Track* track = get_track(track_idx);
    track->sample(time, looping, out);


    /*if(type == ANIM_TYPE_SKELETON) {
        sample_pose(time, data);
    }
    else {
        assert(0);
    }*/

    return time;
}

void Animation::sample_pose(float time, void* out)
{
    /*Pose* pose = reinterpret_cast<Pose*>(out);
    assert(pose);

    for (size_t i = 0; i < tracks.size(); i += 3) {

        Transform transform;
        uint32_t id;

        for (size_t j = i; j < i + 3; ++j) {

            Track& track = tracks[j];
            id = track.get_id();

            switch (tracks[j].get_type()) {
                case TYPE_POSITION: {
                    transform.position = std::get<glm::vec3>(track.sample(time, looping));
                    break;
                }
                case TYPE_ROTATION: {
                    transform.rotation = std::get<glm::quat>(track.sample(time, looping));
                    break;
                }
                case TYPE_SCALE: {
                    transform.scale = std::get<glm::vec3>(track.sample(time, looping));
                    break;
                }
            }
        }

        pose->set_local_transform(id, transform);
    }*/
}

float Animation::adjust_time_to_fit_range(float time)
{
    if (looping) {
        float duration = end_time - start_time;
        if (duration <= 0) { 0.0f; }
        time = fmodf(time - start_time, end_time - start_time);
        if (time < 0.0f) {
            time += end_time - start_time;
        }
        time += start_time;
    }
    else {
        if (time < start_time) {
            time = start_time;
        }
        if (time > end_time) {
            time = end_time;
        }
    }

    return time;
}

void Animation::recalculate_duration()
{
    start_time = 0.0f;
    end_time = 0.0f;

    bool start_set = false;
    bool end_set = false;

    size_t tracks_size = tracks.size();

    for (size_t i = 0; i < tracks_size; ++i) {

        if (!tracks[i].size()) {
            continue;
        }

        float minstart_time = tracks[i].get_start_time();
        float max_end_time = tracks[i].get_end_time();
        if (minstart_time < start_time || !start_set) {
            start_time = minstart_time;
            start_set = true;
        }
        if (max_end_time > end_time || !end_set) {
            end_time = max_end_time;
            end_set = true;
        }
    }

    duration = end_time - start_time;
}

Track* Animation::add_track(int id)
{
    Track track = {};
    track.set_id(id);

    tracks.push_back(track);

    return &tracks[tracks.size() - 1];
}

// Retrieves the Track object for a specific id in the Animation
Track* Animation::operator[](uint32_t id)
{
    return &tracks[id];
}

// Retrieves the Track object for a specific id in the Animation
Track* Animation::get_track(uint32_t i)
{
    return &tracks[i];
}

// Retrieves the Track object for a specific id in the Animation
Track* Animation::get_track_by_id(int id)
{
    for (size_t i = 0, s = tracks.size(); i < s; ++i) {
        if (tracks[i].get_id() == id) {
            return &tracks[i];
        }
    }
    assert(0);
    return nullptr;
}

// getters
std::string& Animation::get_name()
{
    return name;
}

int Animation::get_id_at_index(uint32_t index)
{
    return tracks[index].get_id();
}

uint32_t Animation::get_track_count()
{
    return (uint32_t)tracks.size();
}

float Animation::get_duration()
{
    return duration;
}

float Animation::get_start_time()
{
    return start_time;
}

float Animation::get_end_time()
{
    return end_time;
}

bool Animation::get_looping()
{
    return looping;
}

AnimationType Animation::get_type()
{
    return type;
}

// setters

void Animation::set_type(AnimationType new_type)
{
    type = new_type;
}

void Animation::set_name(const std::string& new_name)
{
    if (new_name.size() > 0) {
        name = new_name;
    }
}

void Animation::set_id_at_index(uint32_t index, int id)
{
    return tracks[index].set_id(id);
}

void Animation::set_looping(bool new_looping)
{
    looping = new_looping;
}
