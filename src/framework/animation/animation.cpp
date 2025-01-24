#include "animation.h"

#include <fstream>

uint32_t Animation::last_animation_id = 0;

Animation::Animation()
{
    name = "AnimationUnnamed_" + std::to_string(last_animation_id++);
    start_time = 0.0f;
    end_time = 0.0f;
}

float Animation::sample(float time, uint32_t track_idx, uint8_t loop, void* out, eInterpolationType interpolation_type)
{
    if (get_duration() == 0.0f) {
        return 0.0f;
    }

    time = adjust_time_to_fit_range(time, loop);

    Track* track = get_track(track_idx);
    track->sample(time, loop, out, interpolation_type);

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

void Animation::serialize(std::ofstream& binary_scene_file)
{
    size_t name_size = name.size();
    binary_scene_file.write(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    binary_scene_file.write(name.c_str(), name_size);

    binary_scene_file.write(reinterpret_cast<char*>(&start_time), sizeof(float));
    binary_scene_file.write(reinterpret_cast<char*>(&end_time), sizeof(float));
    binary_scene_file.write(reinterpret_cast<char*>(&duration), sizeof(float));
    binary_scene_file.write(reinterpret_cast<char*>(&reversed), sizeof(bool));
    binary_scene_file.write(reinterpret_cast<char*>(&type), sizeof(eAnimationType));

    size_t tracks_count = tracks.size();
    binary_scene_file.write(reinterpret_cast<char*>(&tracks_count), sizeof(size_t));

    for (uint32_t i = 0u; i < tracks_count; ++i) {
        Track& t = tracks[i];
        t.serialize(binary_scene_file);
    }
}

void Animation::parse(std::ifstream& binary_scene_file)
{
    size_t name_size = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    name.resize(name_size);
    binary_scene_file.read(&name[0], name_size);

    binary_scene_file.read(reinterpret_cast<char*>(&start_time), sizeof(float));
    binary_scene_file.read(reinterpret_cast<char*>(&end_time), sizeof(float));
    binary_scene_file.read(reinterpret_cast<char*>(&duration), sizeof(float));
    binary_scene_file.read(reinterpret_cast<char*>(&reversed), sizeof(bool));
    binary_scene_file.read(reinterpret_cast<char*>(&type), sizeof(eAnimationType));

    size_t tracks_count = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&tracks_count), sizeof(size_t));
    tracks.resize(tracks_count);

    for (uint32_t i = 0u; i < tracks_count; ++i) {
        Track& t = tracks[i];
        t.parse(binary_scene_file);
    }
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
