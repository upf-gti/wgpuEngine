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
    if (!root_node) {
        root_node = get_parent();
    }
        
    Animation* animation = RendererStorage::get_animation(animation_name);
    if (!animation) {
        spdlog::error("No animation called {}", animation_name);
        return;
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
    current_animation_name = animation_name;

    // sequence with default values
    timeline.frame_max = animation->get_track(0)->size();;

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
            track_data[i] = node->get_property(property_name);
        }
        else {
            spdlog::warn("{} node not found", track_path);
        }
       
        int count = 1;
        int type = -1;
        if (animation->get_track(i)->get_type() == TrackType::TYPE_POSITION) {
            type = 0;
            count = 3;
        }
        else if (animation->get_track(i)->get_type() == TrackType::TYPE_ROTATION) {
            type = 1;
            count = 4;
        }
        else if (animation->get_track(i)->get_type() == TrackType::TYPE_SCALE) {
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
                /*timeline.curve_editor.AddPoint(0, ImVec2(j, p.x));
                timeline.curve_editor.AddPoint(1, ImVec2(j, p.y));
                timeline.curve_editor.AddPoint(2, ImVec2(j, p.z));
                timeline.curve_editor.AddPoint(3, ImVec2(j, p.w));*/
            }
        }
        timeline.tracks.push_back(Timeline::TimelineTrack{ animation->get_track(i)->get_type(), 0, (int)animation->get_track(i)->size(), false, track_path, points });

    }
}

void AnimationPlayer::pause()
{
    playing = !playing;
}

void AnimationPlayer::stop(bool keep_state)
{
    blender.stop();
    playing = false;
}

void AnimationPlayer::update(float delta_time)
{
    if (!playing) {

        Node::update(delta_time);

        return;
    }

    Animation* current_animation = blender.get_current_animation();
    assert(current_animation);

    if (!current_animation->get_looping() && playback >= current_animation->get_duration()) {
        playing = false;
        return;
    }

    // Sample data from the animation and store it at &track_data
    
    playback = blender.update(delta_time * speed, track_data);

    /*
        After sampling, we should have the skeletonInstance joint nodes with the correct
        transforms.. so use those nodes to update the pose of the skeletons
        Setting the model dirty for everyone means that:
            - skeleton_instances will update the pose from its joints when updating
            - nodes that are not joints will automatically set its new model from the animatable properties
    */

    root_node->set_model_dirty(true);

    //for (auto instance : root_node->get_children()) {

    //    MeshInstance3D* node = dynamic_cast<MeshInstance3D*>(instance);

    //    if (!node) {
    //        continue;
    //    }

    //    // skeletal animation case: we get the skeleton pose
    //    // in case we want to process the values and update it manually
    //    if (current_animation->get_type() == anim_type_skeleton) {
    //        skeleton* skeleton = node->get_skeleton();
    //        if (!skeleton) {
    //            continue;
    //        }
    //        pose& pose = skeleton->get_current_pose();
    //        playback = blender.update(delta_time * speed, &pose);
    //    }
    //    else {
    //        // general case: out is not used..
    //        playback = blender.update(delta_time * speed);
    //    }
    //}

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

    if (ImGui::Checkbox("Loop", &looping)) {
        Animation* current_animation = blender.get_current_animation();
        if (current_animation) {
            current_animation->set_looping(looping);
        }
    }

    ImGui::DragFloat("Speed", &speed, 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat("Blend time", &blend_time, 0.1f, 0.0f);
    ImGui::LabelText(std::to_string(playback).c_str(), "Playback");

    // Control buttons
    if (!playing && ImGui::Button("Play")) {
        play(current_animation_name);
    }
    else if (playing && ImGui::Button("Stop")) {
        stop();
    }
    ImGui::SameLine(0, 10);

    if (ImGui::Button("Pause")) {
        pause();
    }

    // Timeline

    if (ImGui::Begin("Timeline")) {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y / 3));
        ImGui::SetWindowPos(ImVec2(0, io.DisplaySize.y - io.DisplaySize.y / 3));

     
        // let's create the sequencer
        static int selected_entry = -1;
        static int first_frame = 0;
        static bool expanded = true;
        static int current_frame = 100;

        ImGui::PushItemWidth(130);
        ImGui::InputInt("Frame Min", &timeline.frame_min);
        ImGui::SameLine();
        ImGui::InputInt("Frame ", &current_frame);
        ImGui::SameLine();
        ImGui::InputInt("Frame Max", &timeline.frame_max);
        ImGui::PopItemWidth();
        Sequencer(&timeline, &current_frame, &expanded, &selected_entry, &first_frame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME);
        // add a UI to edit that particular item
        if (selected_entry != -1)
        {
            const Timeline::TimelineTrack item = timeline.tracks[selected_entry];
            ImGui::Text("I am a %s, please edit me", TrackTypes[item.type]);
            // switch (type) ....
        }
        ImGui::End();
    }
}

void AnimationPlayer::set_speed(float time)
{
    speed = time;
}

void AnimationPlayer::set_blend_time(float time)
{
    blend_time = time;
}

void AnimationPlayer::set_looping(bool loop)
{
    looping = loop;

    Animation* current_animation = blender.get_current_animation();

    if (current_animation)
    {
        current_animation->set_looping(looping);
    }
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

bool AnimationPlayer::is_looping()
{
    return looping;
}

bool AnimationPlayer::is_playing()
{
    return playing;
}
