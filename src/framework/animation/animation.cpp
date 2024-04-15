#include "animation.h"

Animation::Animation()
{
    name = "AnimationUnnamed";
    start_time = 0.0f;
    end_time = 0.0f;
    looping = true;
}

float Animation::sample(float time)
{
    if (get_duration() == 0.0f) {
        return 0.0f;
    }

    time = adjust_time_to_fit_range(time);

    size_t size = tracks.size();

    for (size_t i = 0; i < size; ++i) {

        Track& track = tracks[i];

        // sample the track
        track.sample(time, looping, track.get_data());
    }

    return time;
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
}

Track* Animation::add_track(uint32_t id, void* data)
{
    Track track = {};
    track.set_id(id);
    track.set_data(data);

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
Track* Animation::get_track_by_id(uint32_t id)
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

uint32_t Animation::get_id_at_index(uint32_t index)
{
    return tracks[index].get_id();
}

uint32_t Animation::size()
{
    return (uint32_t)tracks.size();
}

float Animation::get_duration()
{
    return end_time - start_time;
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

// setters

void Animation::set_type(AnimationType new_type)
{
    type = new_type;
}

void Animation::set_name(const std::string& new_name)
{
    name = new_name;
}

void Animation::set_id_at_index(uint32_t index, uint32_t id)
{
    return tracks[index].set_id(id);
}

void Animation::set_looping(bool new_looping)
{
    looping = new_looping;
}
