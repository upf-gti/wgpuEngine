#include "animation_player.h"

#include "graphics/renderer_storage.h"
#include "framework/input.h"

#include "spdlog/spdlog.h"

#include "skeleton_instance_3d.h"

AnimationPlayer::AnimationPlayer(const std::string& n)
{
    name = n;
}

void AnimationPlayer::play(const std::string& animation_name, float custom_blend, float custom_speed, bool from_end)
{
    Animation* animation = RendererStorage::get_animation(animation_name);
    if (!animation) {
        spdlog::error("No animation called {}", animation_name);
        return;
    }

    play(animation, custom_blend, custom_speed, from_end);
}

void AnimationPlayer::play(Animation* animation, float custom_blend, float custom_speed, bool from_end)
{
    if (paused) {
        resume();
        return;
    }

    if (!root_node) {
        root_node = get_parent();
    }

    if (custom_blend >= 0.0f) {
        blend_time = custom_blend;
        blender.fade_to(animation, blend_time);
    }
    else {
        blender.play(animation);
        playback = 0.0f;
    }

    speed = custom_speed;
    playing = true;
    current_animation_name = animation->get_name();

    // sequence with default values
    timeline.frame_max = animation->get_track(0)->size();
    selected_track = -1;

    generate_track_data();
}

void AnimationPlayer::generate_track_data()
{
    track_data.clear();

    Animation* animation = RendererStorage::get_animation(current_animation_name);
    uint32_t num_tracks = animation->get_track_count();
    track_data.resize(num_tracks);
    timeline.tracks.clear();

    // Generate data from tracks
    for (uint32_t i = 0; i < num_tracks; ++i) {

        const std::string& track_path = animation->get_track(i)->get_path();
   
        size_t last_idx = track_path.find_last_of('/');
        const std::string& node_path = track_path.substr(0, last_idx);
        const std::string& property_name = track_path.substr(last_idx + 1);

        // this get_node is overrided by SkeletonInstance, depending on the class
        // it will use hierarchical search or linear search in joints array
        Node* node = root_node->get_node(node_path);

        if (node) {
            track_data[i] = node->get_animatable_property(property_name).property;
        }
        else {
            spdlog::warn("{} node not found", track_path);
        }
       
        int count = 1;
        int type = -1;
        if (animation->get_track(i)->get_type() == eTrackType::TYPE_POSITION) {
            type = 0;
            count = 3;
        }
        else if (animation->get_track(i)->get_type() == eTrackType::TYPE_ROTATION) {
            type = 1;
            count = 4;
        }
        else if (animation->get_track(i)->get_type() == eTrackType::TYPE_SCALE) {
            type = 2;
            count = 3;
        }
        const int c = count;

        std::vector<ImVec2> points[4];
        timeline.curve_editor.SetPointsCount(animation->get_track(i)->size());
        
        for (uint32_t j = 0; j < animation->get_track(i)->size(); j++) {
            if (type == 0 || type == 2) {
                glm::vec3 p = std::get<glm::vec3>(animation->get_track(i)->get_keyframe(j).value);

                points[0].push_back(ImVec2(j, p.x));
                points[1].push_back(ImVec2(j, p.y));
                points[2].push_back(ImVec2(j, p.z));

                timeline.curve_editor.point_count[0] = points[0].size();
                timeline.curve_editor.point_count[1] = points[1].size();
                timeline.curve_editor.point_count[2] = points[2].size();
            }
            else if (type == 1) {
                glm::quat p = std::get<glm::quat>(animation->get_track(i)->get_keyframe(j).value);
                points[0].push_back(ImVec2(j, p.x));
                points[1].push_back(ImVec2(j, p.y));
                points[2].push_back(ImVec2(j, p.z));
                points[3].push_back(ImVec2(j, p.w));

                timeline.curve_editor.point_count[0] = points[0].size();
                timeline.curve_editor.point_count[1] = points[1].size();
                timeline.curve_editor.point_count[2] = points[2].size();
                timeline.curve_editor.point_count[3] = points[3].size();
            }
        }
        timeline.tracks.push_back(Timeline::TimelineTrack{ animation->get_track(i)->get_type(), 0, (int)animation->get_track(i)->size(), false, track_path, points });
    }
}

void AnimationPlayer::resume()
{
    playing = true;
    paused = false;
}

void AnimationPlayer::pause()
{
    playing = false;
    paused = true;
}

void AnimationPlayer::stop(bool keep_state)
{
    playback = 0.0f;
    playing = true; // hack to force update..
    update(0.0f);

    if (!keep_state) {
        blender.stop();
    }

    playing = false;
    paused = false;
}

void AnimationPlayer::update(float delta_time)
{
    Animation* current_animation = blender.get_current_animation();
    if (current_animation == nullptr) {
        return;
    }

    if (!playing) {
        Node::update(delta_time);
        return;
    }

    if (loop_type == ANIMATION_LOOP_NONE && playback >= current_animation->get_duration()) {
        playing = false;
        return;
    }

    bool playing_reverse = current_animation->is_reversed();
    float current_time = playback + delta_time * speed * (playing_reverse ? -1.0f : 1.0f);

    // Sample data from the animation and store it at &track_data   
    playback = blender.update(current_time, loop_type, track_data);

    /*
        After sampling, we should have the skeletonInstance joint nodes with the correct
        transforms.. so use those nodes to update the pose of the skeletons
        Setting the model dirty for everyone means that:
            - skeleton_instances will update the pose from its joints when updating
            - nodes that are not joints will automatically set its new model from the animatable properties
    */

    root_node->set_transform_dirty(true);

    Node::update(delta_time);
}

void AnimationPlayer::render_gui()
{
    if (ImGui::BeginCombo("##current", current_animation_name.c_str()))
    {
        for (auto& instance : RendererStorage::animations)
        {
            bool is_selected = (current_animation_name == instance.first);
            if (ImGui::Selectable(instance.first.c_str(), is_selected)) {
                play(instance.first, blend_time);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    Animation* current_animation = blender.get_current_animation();
    if (current_animation == nullptr)
        return;

    if (ImGui::Combo("Loop Type", (int*)&loop_type, "NONE\0DEFAULT\0REVERSE\0PING_PONG\0")) {
        if (loop_type != ANIMATION_LOOP_NONE) {
            play(current_animation);
        }
    }

    ImGui::DragFloat("Blend time", &blend_time, 0.1f, 0.0f);

    // Timeline
    if (ImGui::Begin("Timeline")) {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y / 3));
        ImGui::SetWindowPos(ImVec2(0, io.DisplaySize.y - io.DisplaySize.y / 3));

        // let's create the sequencer
        static int selected_entry = selected_track;
        static int first_frame = 0;
        static bool expanded = true;
        int current_frame = playback * (timeline.frame_max - timeline.frame_min) / current_animation->get_duration();
        int new_current_frame = current_frame;
        // Control buttons
        ImGui::PushItemWidth(180);
        /*ImGui::InputInt("Frame Min", &timeline.frame_min);
        ImGui::SameLine();*/
        if (ImGui::InputInt("Frame ", &new_current_frame)) {
        }
        ImGui::SameLine();
        /*ImGui::InputInt("Frame Max", &timeline.frame_max);
        ImGui::SameLine();*/
        ImGui::InputFloat("Current time", &playback);
        ImGui::SameLine();
        ImGui::DragFloat("Speed", &speed, 0.1f, -10.0f, 10.0f);
        ImGui::SameLine();

        if (!playing && ImGui::Button("Play")) {
            play(current_animation_name);
        }
        else if (playing && ImGui::Button("Stop")) {
            stop();
        }
        ImGui::SameLine();

        if (ImGui::Button("Pause")) {
            pause();
        }

        ImGui::PopItemWidth();

        Sequencer(&timeline, &new_current_frame, &expanded, &selected_entry, &first_frame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME);
        if (new_current_frame != current_frame) {
            playback = new_current_frame * current_animation->get_duration() / (timeline.frame_max - timeline.frame_min);
            blender.update(playback, loop_type, track_data);
            root_node->set_transform_dirty(true);
        }

        // add a UI to edit that particular item
        if (selected_entry != -1)
        {            
            const Timeline::TimelineTrack item = timeline.tracks[selected_entry];
            ImGui::Text("I am a %s, please edit me", TrackTypes[item.type].c_str());
            //std::cout << "I am a %s, please edit me ( " << TrackTypes[item.type].c_str() << " )" << std::endl;
            size_t last_idx = item.name.find_last_of('/');
            std::string node_path = item.name.substr(0, last_idx);

            Node3D* node = (Node3D*)root_node->get_node(node_path);
            if (!node->is_selected() && !playing) {
                node->select();
            }

            if (selected_entry != selected_track)
            {
                if (selected_track != -1)
                {

                    node_path = timeline.tracks[selected_track].name;
                    last_idx = node_path.find_last_of('/');
                    node_path = node_path.substr(0, last_idx);

                    node = (Node3D*)root_node->get_node(node_path);
                    node->unselect();
                }

                selected_track = selected_entry;
            }

        }
    }

    ImGui::End();
}

void AnimationPlayer::set_speed(float time)
{
    speed = time;
}

void AnimationPlayer::set_blend_time(float time)
{
    blend_time = time;
}

void AnimationPlayer::set_loop_type(uint8_t new_loop_type)
{
    loop_type = new_loop_type;
}

void AnimationPlayer::set_root_node(Node3D* new_root_node)
{
    this->root_node = new_root_node;
}

float AnimationPlayer::get_blend_time()
{
    return blend_time;
}

float AnimationPlayer::get_speed()
{
    return speed;
}

uint8_t AnimationPlayer::get_loop_type()
{
    return loop_type;
}

bool AnimationPlayer::is_playing()
{
    return playing;
}
