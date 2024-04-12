#include "animation.h"

Animation::Animation() {
    name = "No name given";
    start_time = 0.0f;
    end_time = 0.0f;
    looping = true;
}

T Animation::sample(std::vector<T>& out, float time)
{
    if (get_duration() == 0.0f) {
        return 0.0f;
    }

    time = adjust_time_to_fit_range(time);

    size_t size = tracks.size();

    for (size_t i = 0; i < size; ++i) {
        // get the ID of the  track
        // sample the track
        T animated = tracks[i].sample(time, looping);
        // assign the sampled value back to the Pose reference
        out[i] = animated;
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
        time = time + start_time;
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

    bool startSet = false;
    bool endSet = false;

    size_t tracksSize = tracks.size();
    for (size_t i = 0; i < tracksSize; ++i) {
        if (tracks[i].size()) {
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

// retrieves the Track object for a specific id in the Animation
Track& Animation::operator[](unsigned int id)
{
    return get_track(id);
}

// retrieves the Track object for a specific id in the Animation
Track& Animation::get_track(unsigned int id)
{
    for (size_t i = 0, s = tracks.size(); i < s; ++i) {
        if (tracks[i].get_id() == id) {
            return tracks[i];
        }
    }
    // if no qualifying track is found, a new one is created and returned
    tracks.push_back(Track());
    tracks[tracks.size() - 1].set_id(id);
    return tracks[tracks.size() - 1];
}

// getters
std::string& Animation::get_name()
{
    return name;
}

unsigned int Animation::get_id_at_index(unsigned int index)
{
    return tracks[index].get_id();
}

unsigned int Animation::size()
{
    return (unsigned int)tracks.size();
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
void Animation::set_name(const std::string& inNewName)
{
    name = inNewName;
}

void Animation::set_id_at_index(unsigned int index, unsigned int id)
{
    return tracks[index].set_id(id);
}

void Animation::set_looping(bool inLooping)
{
    looping = inLooping;
}
