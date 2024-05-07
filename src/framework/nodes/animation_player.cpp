#include "animation_player.h"

#include "graphics/renderer_storage.h"
#include "framework/input.h"

#include "spdlog/spdlog.h"

#include "skeleton_instance_3d.h"

AnimationPlayer::AnimationPlayer(const std::string& n)
{
    name = n;

    Node::bind("rotation@changed", (FuncQuat)[&](const std::string& signal, glm::quat rotation) {
        if (current_animation_name != "") {
            Animation* animation = RendererStorage::get_animation(current_animation_name);
            int frame_idx = timeline.selected_point.y;
            int track_idx = timeline.selected_point.x;
            timeline.tracks[track_idx].edited_points[frame_idx] = true;

            animation->get_track(track_idx)->get_keyframe(frame_idx).value = rotation;
        }
       
    });

    Node::bind("translation@changed", (FuncVec3)[&](const std::string& signal, glm::vec3 position) {
        if (current_animation_name != "") {
            Animation* animation = RendererStorage::get_animation(current_animation_name);
            int frame_idx = timeline.selected_point.y;
            int track_idx = timeline.selected_point.x;
            timeline.tracks[track_idx].edited_points[frame_idx] = true;

            animation->get_track(track_idx)->get_keyframe(frame_idx).value = position;
        }

    });

    Node::bind("scale@changed", (FuncVec3)[&](const std::string& signal, glm::vec3 scale) {
        if (current_animation_name != "") {
            Animation* animation = RendererStorage::get_animation(current_animation_name);
            int frame_idx = timeline.selected_point.y;
            int track_idx = timeline.selected_point.x;
            timeline.tracks[track_idx].edited_points[frame_idx] = true;

            animation->get_track(track_idx)->get_keyframe(frame_idx).value = scale;
        }

    });
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
        std::vector<bool> edited_points;
        //timeline.curve_editor.SetPointsCount(animation->get_track(i)->size());
        
        for (uint32_t j = 0; j < animation->get_track(i)->size(); j++) {
            if (type == 0 || type == 2) {
                glm::vec3 p = std::get<glm::vec3>(animation->get_track(i)->get_keyframe(j).value);

                points[0].push_back(ImVec2(j, p.x));
                points[1].push_back(ImVec2(j, p.y));
                points[2].push_back(ImVec2(j, p.z));

                /*timeline.curve_editor.point_count[0] = points[0].size();
                timeline.curve_editor.point_count[1] = points[1].size();
                timeline.curve_editor.point_count[2] = points[2].size();*/
            }
            else if (type == 1) {
                glm::quat p = std::get<glm::quat>(animation->get_track(i)->get_keyframe(j).value);
                points[0].push_back(ImVec2(j, p.x));
                points[1].push_back(ImVec2(j, p.y));
                points[2].push_back(ImVec2(j, p.z));
                points[3].push_back(ImVec2(j, p.w));

                /*timeline.curve_editor.point_count[0] = points[0].size();
                timeline.curve_editor.point_count[1] = points[1].size();
                timeline.curve_editor.point_count[2] = points[2].size();
                timeline.curve_editor.point_count[3] = points[3].size();*/
                /*timeline.curve_editor.AddPoint(0, ImVec2(j, p.x));
                timeline.curve_editor.AddPoint(1, ImVec2(j, p.y));
                timeline.curve_editor.AddPoint(2, ImVec2(j, p.z));
                timeline.curve_editor.AddPoint(3, ImVec2(j, p.w));*/
            }
            edited_points.push_back(false);
        }
        timeline.tracks.push_back(Timeline::TimelineTrack{ animation->get_track(i)->get_type(), 0, (int)animation->get_track(i)->size(), false, track_path, points, edited_points });
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
    float current_time = playback + delta_time * speed;
    Animation* current_animation = blender.get_current_animation();
    if (current_animation == nullptr)
        return;

    if (!playing) {
        //current_time = playback;
        Node::update(delta_time);
        return;
    }
    if (!current_animation->get_looping() && playback >= current_animation->get_duration()) {
        playing = false;
        return;
    }

    // Sample data from the animation and store it at &track_data   
    playback = blender.update(current_time, track_data);

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

    Animation* current_animation = blender.get_current_animation();
    if (current_animation == nullptr)
        return;

    if (ImGui::Checkbox("Loop", &looping)) {
        if (current_animation) {
            current_animation->set_looping(looping);
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
        ImGui::BeginDisabled();
        ImGui::InputInt("Frame Max", &timeline.frame_max,0, 0);
        ImGui::EndDisabled();
        ImGui::SameLine();
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
        int selected_frame = timeline.selected_point.y;

        Sequencer(&timeline, &new_current_frame, &expanded, &selected_entry, &first_frame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME);     

        // add a UI to edit that particular item
        if (selected_entry != -1)
        {            
            const Timeline::TimelineTrack item = timeline.tracks[selected_entry];
 
            size_t last_idx = item.name.find_last_of('/');
            std::string node_path = item.name.substr(0, last_idx);
            const std::string& property_name = item.name.substr(last_idx + 1);

            Node3D* node = (Node3D*)root_node->get_node(node_path);
            if (node != nullptr) {
                //select gizmo mode based on track's type
                if (property_name == "translation")
                    node->set_edit_mode(EditModes::TRANSLATE);
                else if (property_name == "rotation")
                    node->set_edit_mode(EditModes::ROTATE);
                else if (property_name == "scale")
                    node->set_edit_mode(EditModes::SCALE);

                //select the node only if the animation is not running
                if (!node->is_selected() && !playing) {
                    node->select();
                }
            }

            // update current frame on select frame
            if(selected_frame != timeline.selected_point.y)
                new_current_frame = timeline.selected_point.y;

            // unselect node if other is selected
            if (selected_entry != selected_track)
            {
                if (selected_track != -1)
                {
                    node_path = timeline.tracks[selected_track].name;
                    last_idx = node_path.find_last_of('/');
                    node_path = node_path.substr(0, last_idx);

                    node = (Node3D*)root_node->get_node(node_path);
                    if (node != nullptr) {
                        node->unselect();
                    }
                }
                selected_track = selected_entry;
            }
        }

        //update animation on change current frame manyally
        if (new_current_frame != current_frame) {
            playback = new_current_frame * current_animation->get_duration() / (timeline.frame_max - timeline.frame_min);
            blender.update(playback, track_data);
            root_node->set_model_dirty(true);
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
