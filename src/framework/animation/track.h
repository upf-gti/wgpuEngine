#pragma once

#include "interpolator.h"
#include "glm/vec2.hpp"

enum eTrackType {
    TYPE_UNDEFINED,
    TYPE_FLOAT, // Set a value in a property, can be interpolated.
    TYPE_VECTOR2, // Vector3 track, can be compressed.
    TYPE_VECTOR3, // Vector3 track, can be compressed.
    TYPE_VECTOR4,
    TYPE_QUAT, // Quaternion track, can be compressed.
    TYPE_METHOD, // Call any method on a specific node.
    TYPE_POSITION,
    TYPE_ROTATION,
    TYPE_SCALE
};

enum eLoopType : uint8_t {
    ANIMATION_LOOP_NONE,
    ANIMATION_LOOP_DEFAULT,
    ANIMATION_LOOP_REVERSE,
    ANIMATION_LOOP_PING_PONG
};

// Collection of keyframes
class Track {

    int id = 0;
    std::vector<Keyframe> keyframes;
    eTrackType type = eTrackType::TYPE_UNDEFINED;
    std::string name = "";
    std::string path = "";

    Interpolator interpolator;

    int frame_index(float time);

    // Takes an input time that is outside the range of the track and adjusts it to be a valid time on the track
    float adjust_time_to_fit_track(float time, bool loop);

public:

    Track();

    int get_id();
    float get_end_time();
    float get_start_time();
    eTrackType get_type() const { return type; };
    const std::string& get_name();
    const std::string& get_path();
    Keyframe& get_keyframe(uint32_t index);
    Interpolator& get_interpolator() { return interpolator; }

    void set_id(int id);
    void set_type(eTrackType new_type) { type = new_type; };
    void set_name(const std::string& new_name);
    void set_path(const std::string& new_path);

    uint32_t size();
    void resize(uint32_t size);

    Keyframe& add_keyframe(const Keyframe& k);

    void serialize(std::ofstream& binary_scene_file);
    void parse(std::ifstream& binary_scene_file);

    // Prameters: time value, if the track is looping or not
    TrackType sample(float time, bool looping, void* out = nullptr, eInterpolationType interpolation_type = eInterpolationType::UNSET);
    Keyframe& operator[](uint32_t index);
};
