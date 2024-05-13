#include "time_tunnel.h"

void TimeTunnel::set_axis(const glm::vec3& axis_direction)
{
    axis = axis_direction;
}

void TimeTunnel::set_stretchiness(const float stretchiness_window)
{
    stretchiness = stretchiness_window;
}

void TimeTunnel::set_current_frame(const uint32_t tc)
{
    current_frame = tc;
}

void TimeTunnel::set_number_frames(const uint32_t count)
{
    number_frames = count;
}

void TimeTunnel::set_threshold(const float threshold_diff)
{
    threshold = threshold_diff;
}

void TimeTunnel::set_gaussian_sigma(const float sigma)
{
    gaussian_sigma = sigma;
}

glm::vec3& TimeTunnel::get_axis()
{
    return axis;
}

float TimeTunnel::get_stretchiness()
{
    return stretchiness;
}

uint32_t TimeTunnel::get_current_frame()
{
    return current_frame;
}

uint32_t TimeTunnel::get_number_frames()
{
    return number_frames;
}

float TimeTunnel::get_threshold()
{
    return threshold;
}

float TimeTunnel::get_gaussian_sigma()
{
    return gaussian_sigma;
}

std::vector<uint32_t> TimeTunnel::get_keyframes()
{
    return keyframes;
}

std::vector<std::vector<glm::vec3>> TimeTunnel::get_trajectories()
{
    return trajectories;
}

std::vector<std::vector<glm::vec3>> TimeTunnel::get_smoothed_trajectories()
{
    return smoothed_trajectories;
}

std::vector<glm::vec3> TimeTunnel::compute_gaussian_smoothed_trajectory(std::vector<glm::vec3>& trajectory)
{
    float sigma = gaussian_sigma / (2 * PI);
    std::vector<glm::vec3> smoothed_trajectory;
    // Iterate over each point in the trajectory
    for (size_t i = 0; i < trajectory.size(); ++i) {
        float sum_weights = 0.f;
        glm::vec3 smoothed_point(0.0f);

        // Apply Gaussian smoothing to neighboring points within a certain window
        for (size_t j = 0; j < trajectory.size(); ++j) {
            float weight = gaussian_filter(fabs((float)(i)-(float)(j)), sigma);
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

glm::vec3 TimeTunnel::compute_smoothed_trajectory_position(float t, float max_diff_t, std::vector<glm::vec3>& trajectory, std::vector<glm::vec3>& smoothed_trajectory)
{
    
    float p = -glm::pow((t - max_diff_t), 2.0f) / glm::pow(gaussian_sigma, 2.0f);
    float alpha = glm::exp(p);

    if (trajectory.size() <= t)
    {
        trajectory.push_back(trajectory[t-1]);
    }
    smoothed_trajectory[t] = alpha * trajectory[t] + (1 - alpha) * smoothed_trajectory[t];
    
    return smoothed_trajectory[t];
}

glm::vec3 TimeTunnel::compute_trajectory_position(float t, const glm::vec3& position)
{
    return position + stretchiness * (t - current_frame) * axis;
}

std::vector<std::vector<glm::vec3>> TimeTunnel::compute_trajectories(std::vector<Track*>& tracks)
{
    trajectories.clear();
    magnitudes.clear();
    for (auto& track : tracks)
    {
        std::vector<glm::vec3> trajectory;
        std::vector<float> magnitudes_t;

        for (size_t t = 0; t < track->size(); t++)
        {
            if (track->get_type() == TrackType::TYPE_POSITION)
            {
                glm::vec3 position = std::get<glm::vec3>(track->get_keyframe(t).value);

                // Compute trajectory position in frame t centered on current frame
                glm::vec3 Tk = compute_trajectory_position(t, position);
                trajectory.push_back(Tk);

                // Compute magnitue from joint trajectory position
                float magnitude = glm::length(Tk);
                magnitudes_t.push_back(magnitude);               
            }
        }
        trajectories.push_back(trajectory);
        magnitudes.push_back(magnitudes_t);
    }

    // Compute sum of magnitudes from all joints
    for (size_t t = 0; t < magnitudes[0].size(); t++)
    {
        float sum_magnitudes = 0;
        for (size_t k = 0; k < magnitudes.size(); k++)
        {
            sum_magnitudes += magnitudes[k].size() <= t ? magnitudes[k][magnitudes[k].size() - 1] : magnitudes[k][t];
        }
        total_magnitudes.push_back(sum_magnitudes);
    }

    return trajectories;
}

std::vector<uint32_t> TimeTunnel::extract_keyframes(std::vector<Track*>& tracks)
{
    compute_trajectories(tracks);

    keyframes.clear();

    size_t frames = tracks[0]->size();
    size_t count = tracks.size();
    std::vector<std::vector<glm::vec3>> sT_k(count);

    int t_m = -1; // selected keyframe
    //keyframes.push_back(t_m);

    while (keyframes.size() < number_frames)
    {
        float max_diff = -1.f;
        std::vector<float> d;
        for (size_t t = 0; t < frames; t++) // frames
        {
            float total_diff = 0.0f;

            // Sum for all joints: d(t) += wk * ||Tk(t) - sTk(t)||
            for (size_t k = 0; k < count; k++) // joints/nodes
            {
                if (!sT_k[k].size())
                {
                    sT_k[k].resize(count);
                    // Compute smooth trajectory positions using a gaussian filter
                    sT_k[k] = compute_gaussian_smoothed_trajectory(trajectories[k]);
                }
                else if(keyframes.size()>1){
                    sT_k[k][t] = compute_smoothed_trajectory_position(t, t_m, trajectories[k], sT_k[k]);
                }

                glm::vec3 T_k_position = trajectories[k][t];
                glm::vec3 sT_k_position = sT_k[k][t];
                float w = magnitudes[k][t] / total_magnitudes[t];
                glm::vec3 diff = T_k_position - sT_k_position;
                total_diff += w * glm::length(diff);
            }
            d.push_back(total_diff);
        }

        // select the time with the largest difference among all the frames        
        for (size_t i = 0; i < d.size(); i++)
        {
            if (d[i] > max_diff )
            {
                max_diff = d[i];
                t_m = i;
            }
        }

        if (t_m > -1)
        {
            keyframes.push_back(t_m);
        }

        if (max_diff < threshold)
        {
            break;
        }
    }
    smoothed_trajectories = sT_k;
    std::sort(keyframes.begin(), keyframes.end());    
    return keyframes;
}


void TimeTunnel::clear()
{
    trajectories.clear();
    smoothed_trajectories.clear();
    keyframes.clear();
    magnitudes.clear();
    total_magnitudes.clear();
}
