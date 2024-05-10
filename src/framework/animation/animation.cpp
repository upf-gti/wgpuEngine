#include "animation.h"
#include <algorithm> 

uint32_t Animation::last_animation_id = 0;

Animation::Animation()
{
    name = "AnimationUnnamed_" + std::to_string(last_animation_id++);
    start_time = 0.0f;
    end_time = 0.0f;
    looping = true;
}

float Animation::sample(float time, uint32_t index, void* out)
{
    if (get_duration() == 0.0f) {
        return 0.0f;
    }

    time = adjust_time_to_fit_range(time);

    if(time >= get_duration())
    {
    // clamp the time in the track keyframes range
        return time;       
    }

    Track* track = get_track(index);
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

glm::vec3 Animation::compute_trajectory(Track& track, const glm::vec3& axis, float stretchiness, uint32_t current_frame, uint32_t frame)
{
    std::vector<glm::vec3> trajectory;

    if (track.get_type() == TrackType::TYPE_POSITION)
    {        
        glm::vec3 position = std::get<glm::vec3>(track[frame].value);
        // trajectory(t) = original_position(t) + stretchiness * (t - evaluated_time) * tunnel_direction
        glm::vec3 a = stretchiness * ((float)frame - (float)current_frame) * axis;
        position = position + a;
        return position;
        
    }
    return glm::vec3();
}

glm::vec3 Animation::compute_smoothed_trajectory(glm::vec3& trajectory, glm::vec3& smoothed_trajectory, float window, uint32_t current_frame, uint32_t frame)
{                      
    float p = -glm::pow((frame - current_frame), 2) / glm::pow(window, 2);
    float alpha = glm::exp(p);
            
    smoothed_trajectory = alpha * trajectory + (1 - alpha) * smoothed_trajectory;
     
    return smoothed_trajectory;
}

std::vector<glm::vec3> Animation::compute_gaussian_smoothed_trajectory(std::vector<glm::vec3>& trajectory, float sigma)
{
    std::vector<glm::vec3> smoothed_trajectory;
    // Iterate over each point in the trajectory
    for (size_t i = 0; i < trajectory.size(); ++i) {
        float sum_weights = 0.f;
        glm::vec3 smoothed_point(0.0f);

        // Apply Gaussian smoothing to neighboring points within a certain window
        for (size_t j = 0; j < trajectory.size(); ++j) {
            float weight = gaussian_filter(fabs((float)(i) - (float)(j)), sigma);
            sum_weights += weight;
            smoothed_point += trajectory[j] * weight;           
        }

        // Normalize the smoothed point
        smoothed_point /= sum_weights;
        
        // Add the smoothed point to the output trajectory
        smoothed_trajectory.push_back(smoothed_point);
    }
    return smoothed_trajectory;
}
//float Animation::compute_difference(uint32_t joint_idx, float t, std::map<std::string, glm::vec3>& joint_tracks)
//{
//    glm::vec3 direction(1, 0, 0);
//    float stretchiness = 1.f;
//
//    glm::vec3 position = std::get<glm::vec3>(tracks[joint_idx][t].value);
//
//    glm::vec3 trajectory = compute_trajectory(tracks[joint_idx], direction, stretchiness, t)[t];
//
//    float joint_magnitude =
//    float joints_magnitude = 0;
//    for (auto& joint : joint_tracks)
//    {
//        glm::vec3 position = compute_trajectory(tracks[joint.second.y], direction, stretchiness, t)[t];
//        joints_magnitude += glm::length(position);
//    }
//
//}

void Animation::compute_keyposes(uint32_t current_frame, float stretchiness, float window_constant, int min_f, float sigma, glm::vec3 direction)
{
    if (type != ANIM_TYPE_SKELETON)
        return;

    keyposes.clear();
    std::map<std::string, glm::vec3> joint_tracks;

    for (size_t i = 0; i < tracks.size(); i++)
    {
        Track* track = get_track(i);

        const std::string& name = track->get_name();
        size_t last_idx = name.find_last_of('/');
        const std::string& joint_name = name.substr(0, last_idx);
        const std::string& property_name = name.substr(last_idx + 1);

        std::map<std::string, glm::vec3>::iterator it = joint_tracks.find(joint_name);
        glm::vec3 indices = glm::vec3(-1);

        if (it != joint_tracks.end())
            indices = it->second;

        if (property_name == "translation" || property_name == "translate" || property_name == "position" || track->get_type() == TrackType::TYPE_POSITION) {
            indices.x = i;
        }
        else if (property_name == "rotation" || track->get_type() == TrackType::TYPE_ROTATION) {
            indices.y = i;
        }
        else if (property_name == "scale" || track->get_type() == TrackType::TYPE_SCALE) {
            indices.z = i;
        }
        joint_tracks[joint_name] = indices;
    }

    // Assume all tracks have same number of frames
    const std::vector<float> computed_times = get_track(0)->get_times();
    uint32_t lastIndex = computed_times.size() - 1;


    // Compute differences for all active joints for all frames
    float max_diff = -1;
    int selected = current_frame;
    keyposes.push_back(selected);


    std::vector<std::vector<glm::vec3>> joints_trajectories(computed_times.size());
    std::vector<std::vector<glm::vec3>> joints_smoothed_trajectories(computed_times.size());
    std::vector< std::vector<float>> joints_magnitudes(computed_times.size());
    std::vector<float> weights(computed_times.size());
    std::vector<float> sum_joints_magnitude(computed_times.size());

    ////Init trajectoryes and magnitudes
    for (size_t t = 0; t < computed_times.size(); t++)
    {
        sum_joints_magnitude[t] = 0;
        joints_trajectories[t].resize(joint_tracks.size());
        joints_magnitudes[t].resize(joint_tracks.size());

        int i = 0;
        for (auto& joint : joint_tracks)
        {
            // Compute trajectory position in frame t centered on current frame
            joints_trajectories[t][i] = compute_trajectory(tracks[joint.second.x], direction, stretchiness, current_frame, t);

            // Compute magnitue from joint trajectory position
            float magnitude = glm::length(joints_trajectories[t][i]);
            joints_magnitudes[t][i] = magnitude;
            // Compute sum of magnitudes from all joints
            sum_joints_magnitude[t] += magnitude;

            i++;
        }
        
        // Compute smooth trajectory positions using a gaussian filter
        joints_smoothed_trajectories[t] = compute_gaussian_smoothed_trajectory(joints_trajectories[t], sigma);       
    }

    while (keyposes.size() < min_f)
    {
        float max_diff = -1;
        float total_diff = 0;
        std::vector<float> d;

        ////Init trajectoryes and magnitudes
        //for (size_t t = 0; t < computed_times.size(); t++)
        //{
        //    sum_joints_magnitude[t] = 0;
        //    joints_trajectories[t].clear();
        //    joints_magnitudes[t].clear();
        //    joints_trajectories[t].resize(joint_tracks.size());
        //    joints_magnitudes[t].resize(joint_tracks.size());

        //    int i = 0;
        //    for (auto& joint : joint_tracks)
        //    {
        //        // Compute trajectory position in frame t centered on current frame
        //        joints_trajectories[t][i] = compute_trajectory(tracks[joint.second.x], direction, stretchiness, selected, t);

        //        // Compute magnitue from joint trajectory position
        //        float magnitude = glm::length(joints_trajectories[t][i]);
        //        joints_magnitudes[t][i] = magnitude;
        //        // Compute sum of magnitudes from all joints
        //        sum_joints_magnitude[t] += magnitude;

        //        i++;
        //    }

        //    // Compute smooth trajectory positions using a gaussian filter
        //    joints_smoothed_trajectories[t] = compute_gaussian_smoothed_trajectory(joints_trajectories[t], sigma);
        //}

        for (size_t t = 0; t < computed_times.size(); t++)
        {
            total_diff = 0;
            // Sum for all joints: d(t) += wk * ||Tk(t) - sTk(t)||
            for (size_t i = 0; i < joints_trajectories[t].size(); i++)
            {
                glm::vec3 joint_trajectory_position = joints_trajectories[t][i];
                glm::vec3 joint_trajectory_smoothed_position = joints_smoothed_trajectories[t][i];
                float w = joints_magnitudes[t][i] / sum_joints_magnitude[t];
                glm::vec3 diff = joint_trajectory_position - joint_trajectory_smoothed_position;
                total_diff += w * glm::length(diff);
            }
            d.push_back(total_diff);

        }
        // select the time with the largest difference among all the frames        
        for(size_t i = 0; i < d.size(); i++)
        {
            if (d[i] > max_diff && std::find(keyposes.begin(), keyposes.end(), i) == keyposes.end())
            {
                max_diff = d[i];
                selected = i;
            }
        }
        if (selected > -1)
        {
            keyposes.push_back(selected);
            for (size_t t = 0; t < computed_times.size(); t++)
            {
                // Compute smooth trajectory positions using a gaussian filter
                joints_smoothed_trajectories[t] = compute_gaussian_smoothed_trajectory(joints_trajectories[t], sigma);

                for (size_t i = 0; i < joints_trajectories[t].size(); i++)
                {
                    // Compute smooth trajectory position in frame t centered on current frame
                    joints_smoothed_trajectories[t][i] = compute_smoothed_trajectory(joints_trajectories[t][i], joints_smoothed_trajectories[t][i], window_constant, t, selected);
                }
            }
        }
        //if (total_diff > max_diff && std::find(keyposes.begin(), keyposes.end(), t) == keyposes.end())
        //{
        //    max_diff = total_diff;
        //    selected = t;
        //}

        //// update smoothed trajectory
        //if (!keyposes.size() || std::find(keyposes.begin(), keyposes.end(), selected) == keyposes.end())
        //{
        //    keyposes.push_back(selected);
        //    //compute_smoothed_trajectory(joints_trajectories, joints_smoothed_trajectories, 5, selected);
        //}
        if (max_diff < 0.0005 && max_diff > 0)
            break;
    }

    std::sort(keyposes.begin(), keyposes.end());
    /*for (size_t i = 0; i < tracks.size(); i++)
    {
        std::vector<Keyframe> keyframes;
        for(size_t j = 0; j < keyposes.size(); j++)
        {            
            keyframes.push_back(tracks[i].get_keyframe(keyposes[j]));
        }
        tracks[i].set_keyframes(keyframes);
    }*/
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

// Retrieves the Track object for a specific track path in the Animation
Track* Animation::get_track_by_path(const std::string& path)
{
    for (size_t i = 0, s = tracks.size(); i < s; ++i) {
        if (tracks[i].get_path() == path) {
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

const std::vector<uint32_t>& Animation::get_keyposes()
{
    return keyposes;
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

void Animation::remove_frame(uint32_t index)
{
    for (auto& track : tracks)
    {
        track.remove_keyframe(index);
    }
}
