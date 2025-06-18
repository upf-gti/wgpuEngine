#include "track.h"

#include "glm/gtx/compatibility.hpp"

#include <algorithm>
#include <fstream>

Track::Track()
{
    
}

int Track::get_id() const
{
    return id;
}

void Track::set_id(int index)
{
    id = index;
}

void Track::set_name(const std::string& new_name)
{
    name = new_name;
}

void Track::set_path(const std::string& new_path)
{
    path = new_path;
}

float Track::get_start_time() const
{
    return keyframes[0].time;
}

float Track::get_end_time() const
{
    return keyframes[keyframes.size() - 1].time;
}

const std::string& Track::get_name() const
{
    return name;
}

const std::string& Track::get_path() const
{
    return path;
}

Keyframe& Track::get_keyframe(uint32_t index)
{
    if (index >= keyframes.size()) {
        assert(0);
    }

    return keyframes[index];
}

int Track::get_keyframe_index(float time)
{
    auto it = std::find_if(keyframes.begin(), keyframes.end(), [time](const Keyframe& kf) {
        return std::abs(time - kf.time) < 0.00001f;
    });

    if (it == keyframes.end()) {
        return -1;
    }

    uint32_t idx = it - keyframes.begin();
    return idx;
}

uint32_t Track::add_keyframe(const Keyframe& k, bool sort)
{
    uint32_t idx = keyframes.size();

    keyframes.push_back(k);

    if (sort) {
        std::sort(keyframes.begin(), keyframes.end(), [](const Keyframe& key0, const Keyframe& key1) {
            return key0.time < key1.time;
        });

        idx = get_keyframe_index(k.time);
    }

    if (std::holds_alternative<float>(k.value)) {
        type = eTrackType::TYPE_FLOAT;
    }
    else if (std::holds_alternative<glm::vec3>(k.value)) {
        type = eTrackType::TYPE_VECTOR3;
    }
    else if (std::holds_alternative<glm::vec4>(k.value)) {
        type = eTrackType::TYPE_VECTOR4;
    }
    else if (std::holds_alternative<glm::quat>(k.value)) {
        type = eTrackType::TYPE_ROTATION;
    }

    return idx;
}

void Track::delete_keyframe(int keyframe_idx)
{
    keyframes.erase(keyframes.begin() + keyframe_idx);
}

// call sample_constant, sample_linear, or sample_cubic, depending on the track type.
TrackType Track::sample(float time, bool looping, Node::AnimatableProperty* out, eInterpolationType interpolation_type)
{
    float track_time = adjust_time_to_fit_track(time, looping);
    int frame_idx = frame_index(time);

    eInterpolationType prev_type = interpolator.get_type();

    if (interpolation_type != INTERPOLATION_UNSET) {
        interpolator.set_type(interpolation_type);
    }

    TrackType r = interpolator.interpolate(keyframes, track_time, frame_idx, looping);

    interpolator.set_type(prev_type);

    if (out)
    {
        void* property = out->property;

        bool changed = false;

        // TODO: Support the rest of types..

        if (std::holds_alternative<float>(r)) {
            float* p = reinterpret_cast<float*>(property);
            float old_value = *p;
            *p = std::get<float>(r);
            changed |= (old_value != *p);
        }
        else if (std::holds_alternative<glm::vec3>(r)) {
            glm::vec3* p = reinterpret_cast<glm::vec3*>(property);
            glm::vec3 old_value = *p;
            *p = std::get<glm::vec3>(r);
            changed |= (old_value != *p);
        }
        else if (std::holds_alternative<glm::vec4>(r)) {
            glm::vec4* p = reinterpret_cast<glm::vec4*>(property);
            glm::vec4 old_value = *p;
            *p = std::get<glm::vec4>(r);
            changed |= (old_value != *p);
        }
        else if (std::holds_alternative<glm::quat>(r)) {
            glm::quat* p = reinterpret_cast<glm::quat*>(property);
            glm::quat old_value = *p;
            *p = std::get<glm::quat>(r);
            changed |= (old_value != *p);
        }

        if (changed && out->fn) {
            out->fn();
        }
    }

    return r;
}

Keyframe& Track::operator[](uint32_t index)
{
    return get_keyframe(index);
}

// Size of the keyframes vector
void Track::resize(uint32_t size)
{
    keyframes.resize(size);
}

uint32_t Track::size()
{
    return (uint32_t)keyframes.size();
}

// Return the frame immediately before that time (on the left)
int Track::frame_index(float time)
{
    uint32_t size = (uint32_t)keyframes.size();

    if (size <= 1) {
        return 0;
    }

    for (int i = (int)size - 1; i >= 0; --i) {
        if (time >= keyframes[i].time) {
            return i;
        }
    }

    // Invalid code, we should not reach here!
    return -1;
}

// Adjusts the time to be in the range of the start/end keyframes of the track.
float Track::adjust_time_to_fit_track(float time, bool looping)
{
    uint32_t size = (uint32_t)keyframes.size();

    if (size <= 1) {
        return 0.0f;
    }

    float start_time = keyframes[0].time;
    float end_time = keyframes[size - 1].time;
    float duration = end_time - start_time;

    if (duration <= 0.0f) {
        return 0.0f;
    }

    if (looping) {
        time = fmodf(time - start_time, end_time - start_time);
        if (time < 0.0f) {
            time += end_time - start_time;
        }
        time = time + start_time;
    }
    else {
        if (time <= keyframes[0].time) {
            time = start_time;
        }
        if (time >= keyframes[size - 1].time) {
            time = end_time;
        }
    }

    return time;
}

void Track::serialize(std::ofstream& binary_scene_file)
{
    size_t name_size = name.size();
    binary_scene_file.write(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    binary_scene_file.write(name.c_str(), name_size);

    size_t path_size = path.size();
    binary_scene_file.write(reinterpret_cast<char*>(&path_size), sizeof(size_t));
    binary_scene_file.write(path.c_str(), path_size);

    binary_scene_file.write(reinterpret_cast<char*>(&id), sizeof(int));
    binary_scene_file.write(reinterpret_cast<char*>(&type), sizeof(eTrackType));

    eInterpolationType interpolator_type = interpolator.get_type();
    binary_scene_file.write(reinterpret_cast<char*>(&interpolator_type), sizeof(eInterpolationType));

    size_t keyframes_count = keyframes.size();
    binary_scene_file.write(reinterpret_cast<char*>(&keyframes_count), sizeof(size_t));

    for (uint32_t i = 0u; i < keyframes_count; ++i) {
        const Keyframe& t = keyframes[i];

        float time = t.time;
        binary_scene_file.write(reinterpret_cast<char*>(&time), sizeof(float));

        if (std::holds_alternative<float>(t.value)) {
            float value = std::get<float>(t.value);
            binary_scene_file.write(reinterpret_cast<char*>(&value), sizeof(float));
        }
        else if (std::holds_alternative<glm::vec3>(t.value)) {
            glm::vec3 value = std::get<glm::vec3>(t.value);
            binary_scene_file.write(reinterpret_cast<char*>(&value), sizeof(glm::vec3));
        }
        else if (std::holds_alternative<glm::vec4>(t.value)) {
            glm::vec4 value = std::get<glm::vec4>(t.value);
            binary_scene_file.write(reinterpret_cast<char*>(&value), sizeof(glm::vec4));
        }
        else if (std::holds_alternative<glm::quat>(t.value)) {
            glm::quat value = std::get<glm::quat>(t.value);
            binary_scene_file.write(reinterpret_cast<char*>(&value), sizeof(glm::quat));
        }

        if (std::holds_alternative<float>(t.in)) {
            float in = std::get<float>(t.in);
            binary_scene_file.write(reinterpret_cast<char*>(&in), sizeof(float));
        }
        else if (std::holds_alternative<glm::vec3>(t.in)) {
            glm::vec3 in = std::get<glm::vec3>(t.in);
            binary_scene_file.write(reinterpret_cast<char*>(&in), sizeof(glm::vec3));
        }
        else if (std::holds_alternative<glm::vec4>(t.in)) {
            glm::vec4 in = std::get<glm::vec4>(t.in);
            binary_scene_file.write(reinterpret_cast<char*>(&in), sizeof(glm::vec4));
        }
        else if (std::holds_alternative<glm::quat>(t.in)) {
            glm::quat in = std::get<glm::quat>(t.in);
            binary_scene_file.write(reinterpret_cast<char*>(&in), sizeof(glm::quat));
        }

        if (std::holds_alternative<float>(t.out)) {
            float out = std::get<float>(t.out);
            binary_scene_file.write(reinterpret_cast<char*>(&out), sizeof(float));
        }
        else if (std::holds_alternative<glm::vec3>(t.out)) {
            glm::vec3 out = std::get<glm::vec3>(t.out);
            binary_scene_file.write(reinterpret_cast<char*>(&out), sizeof(glm::vec3));
        }
        else if (std::holds_alternative<glm::vec4>(t.out)) {
            glm::vec4 out = std::get<glm::vec4>(t.out);
            binary_scene_file.write(reinterpret_cast<char*>(&out), sizeof(glm::vec4));
        }
        else if (std::holds_alternative<glm::quat>(t.out)) {
            glm::quat out = std::get<glm::quat>(t.out);
            binary_scene_file.write(reinterpret_cast<char*>(&out), sizeof(glm::quat));
        }
    }
}

void Track::parse(std::ifstream& binary_scene_file)
{
    size_t name_size = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&name_size), sizeof(size_t));
    name.resize(name_size);
    binary_scene_file.read(&name[0], name_size);

    size_t path_size = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&path_size), sizeof(size_t));
    path.resize(path_size);
    binary_scene_file.read(&path[0], path_size);

    binary_scene_file.read(reinterpret_cast<char*>(&id), sizeof(int));
    binary_scene_file.read(reinterpret_cast<char*>(&type), sizeof(eTrackType));

    eInterpolationType interpolator_type = INTERPOLATION_UNSET;
    binary_scene_file.read(reinterpret_cast<char*>(&interpolator_type), sizeof(eInterpolationType));
    interpolator.set_type(interpolator_type);

    size_t keyframes_count = 0;
    binary_scene_file.read(reinterpret_cast<char*>(&keyframes_count), sizeof(size_t));
    keyframes.resize(keyframes_count);

    for (uint32_t i = 0u; i < keyframes_count; ++i) {
        Keyframe& t = keyframes[i];

        binary_scene_file.read(reinterpret_cast<char*>(&t.time), sizeof(float));

        if (type == eTrackType::TYPE_FLOAT) {
            float value = 0.0f;
            binary_scene_file.read(reinterpret_cast<char*>(&value), sizeof(float));
            t.value = value;
        }
        else if (type == eTrackType::TYPE_POSITION || type == eTrackType::TYPE_SCALE || type == eTrackType::TYPE_VECTOR3) {
            glm::vec3 value = {};
            binary_scene_file.read(reinterpret_cast<char*>(&value), sizeof(glm::vec3));
            t.value = value;
        }
        else if (type == eTrackType::TYPE_VECTOR4) {
            glm::vec4 value = {};
            binary_scene_file.read(reinterpret_cast<char*>(&value), sizeof(glm::vec4));
            t.value = value;
        }
        else if (type == eTrackType::TYPE_ROTATION) {
            glm::quat value = {};
            binary_scene_file.read(reinterpret_cast<char*>(&value), sizeof(glm::quat));
            t.value = value;
        }

        // TODO: Serialize also the type of tangents: for now in animation editor we always use "float"
        float in = 0.0f;
        binary_scene_file.read(reinterpret_cast<char*>(&in), sizeof(float));
        t.in = in;

        float out = 0.0f;
        binary_scene_file.read(reinterpret_cast<char*>(&out), sizeof(float));
        t.out = out;
    }
}
