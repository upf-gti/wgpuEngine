#include "animation.h"

uint32_t Animation::last_animation_id = 0;

Animation::Animation()
{
    name = "AnimationUnnamed_" + std::to_string(last_animation_id++);
    start_time = 0.0f;
    end_time = 0.0f;
}

float Animation::sample(float time, uint32_t track_idx, uint8_t loop, void* out)
{
    if (get_duration() == 0.0f) {
        return 0.0f;
    }

    time = adjust_time_to_fit_range(time, loop);

    Track* track = get_track(track_idx);
    track->sample(time, loop, out);

    return time;
}

float Animation::adjust_time_to_fit_range(float time, uint8_t loop)
{
    if (duration <= 0) { return 0.0f; }

    switch (loop)
    {
    case ANIMATION_LOOP_NONE:
        if (time < start_time) {
            time = start_time;
        }
        else if (time > end_time) {
            time = end_time;
        }
        break;
    case ANIMATION_LOOP_DEFAULT:
        reversed = false;
        if (time > end_time) {
            time = start_time + fmod(time - start_time, duration);
        }
        break;
    case ANIMATION_LOOP_REVERSE:
        reversed = true;
        if (time < start_time) {
            time = end_time;
        }
        break;
    case ANIMATION_LOOP_PING_PONG:
        if (reversed) {
            if (time < start_time) {
                time = start_time;
                reversed = false;
            }
        }
        else {
            if (time > end_time) {
                time = start_time + fmod(time - start_time, duration);
                reversed = true;
            }
        }
        break;
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

eAnimationType Animation::get_type()
{
    return type;
}

bool Animation::is_reversed()
{
    return reversed;
}

// setters

void Animation::set_type(eAnimationType new_type)
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
