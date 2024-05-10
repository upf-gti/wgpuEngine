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
            int track_idx = timeline.selected_point.x;
            int frame_idx = timeline.tracks[track_idx].points[0][timeline.selected_point.y].x;

            if (frame_idx == -1 || track_idx == -1)
                return;

            timeline.tracks[track_idx].edited_points[timeline.selected_point.y] = true;

            //animation->get_track(track_idx)->get_keyframe(frame_idx).value = rotation;
            std::vector<uint32_t> keyposes = animation->get_keyposes();
            if(keyposes.size())
            {
                int prev = timeline.selected_point.y - 1 >= 0 ? timeline.selected_point.y - 1 : 0;
                int next = timeline.selected_point.y + 1 < keyposes.size() ? timeline.selected_point.y + 1 : keyposes.size() - 1;
                
                animation->get_track(track_idx)->update_value(frame_idx, (T)rotation, glm::vec2(keyposes[prev], keyposes[next]));
            }
            else {
                animation->get_track(track_idx)->get_keyframe(frame_idx).value = rotation;
            }
        }
       
    });

    Node::bind("translation@changed", (FuncVec3)[&](const std::string& signal, glm::vec3 position) {
        if (current_animation_name != "") {
            int track_idx = timeline.selected_point.x;

            if(timeline.selected_point.y == -1 || track_idx == -1)
                return;

            int frame_idx = timeline.tracks[track_idx].points[0][timeline.selected_point.y].x;

            timeline.tracks[track_idx].edited_points[timeline.selected_point.y] = true;

            Animation* animation = RendererStorage::get_animation(current_animation_name);

            std::vector<uint32_t> keyposes = animation->get_keyposes();
            if (keyposes.size())
            {
                int prev = timeline.selected_point.y - 1 >= 0 ? timeline.selected_point.y - 1 : 0;
                int next = timeline.selected_point.y + 1 < keyposes.size() ? timeline.selected_point.y + 1 : keyposes.size() - 1;

                animation->get_track(track_idx)->update_value(frame_idx, (T)position, glm::vec2(keyposes[prev], keyposes[next]));
            }
            else {
                animation->get_track(track_idx)->get_keyframe(frame_idx).value = position;
            }
        }

    });

    Node::bind("scale@changed", (FuncVec3)[&](const std::string& signal, glm::vec3 scale) {
        if (current_animation_name != "") {
            int track_idx = timeline.selected_point.x;

            if (timeline.selected_point.y == -1 || track_idx == -1)
                return;

            int frame_idx = timeline.tracks[track_idx].points[0][timeline.selected_point.y].x;

            timeline.tracks[track_idx].edited_points[timeline.selected_point.y] = true;

            Animation* animation = RendererStorage::get_animation(current_animation_name);

            std::vector<uint32_t> keyposes = animation->get_keyposes();
            if (keyposes.size())
            {
                int prev = timeline.selected_point.y - 1 >= 0 ? timeline.selected_point.y - 1 : 0;
                int next = timeline.selected_point.y + 1 < keyposes.size() ? timeline.selected_point.y + 1 : keyposes.size() - 1;

                animation->get_track(track_idx)->update_value(frame_idx, (T)scale, glm::vec2(keyposes[prev], keyposes[next]));
            }
            else {
                animation->get_track(track_idx)->get_keyframe(frame_idx).value = scale;
            }
        }
    });

    Node::bind("transform@changed", [&](const std::string& signal, Transform t) {
        if (current_animation_name != "") {
            int track_idx = timeline.selected_point.x;

            if (timeline.selected_point.y == -1 || track_idx == -1)
                return;

            int frame_idx = timeline.tracks[track_idx].points[0][timeline.selected_point.y].x;


            Animation* animation = RendererStorage::get_animation(current_animation_name);

            const std::string& track_path = animation->get_track(track_idx)->get_path();

            size_t last_idx = track_path.find_last_of('/');
            const std::string& node_path = track_path.substr(0, last_idx);
            const std::string& property_name = track_path.substr(last_idx + 1);
           
            Track* track = animation->get_track_by_path(node_path + "/translation");

            if (track)
            {
                track->get_keyframe(frame_idx).value = t.position;
            }
            
            track = animation->get_track_by_path(node_path + "/rotation");
            if (track)
            {
                track->get_keyframe(frame_idx).value = t.rotation;
            }

            track = animation->get_track_by_path(node_path + "/scale");
            if (track)
            {
                track->get_keyframe(frame_idx).value = t.scale;
            }

            for (size_t i = 0; i < timeline.tracks.size(); i++) {
                if (timeline.tracks[i].name == node_path + "/translation") {
                    timeline.tracks[i].edited_points[timeline.selected_point.y] = true;
                }
                else if (timeline.tracks[i].name == node_path + "/rotation") {
                    timeline.tracks[i].edited_points[timeline.selected_point.y] = true;
                }
                else if (timeline.tracks[i].name == node_path + "/scale") {
                    timeline.tracks[i].edited_points[timeline.selected_point.y] = true;
                }
            }
        }
    });
}

void AnimationPlayer::play(const std::string& animation_name, float custom_blend, float custom_speed, bool from_end)
{
    if (!root_node) {
        root_node = get_parent();
    }
        
    current_animation = RendererStorage::get_animation(animation_name);
    if (!current_animation) {
        spdlog::error("No animation called {}", animation_name);
        return;
    }

    if (custom_blend >= 0.0f) {
        blend_time = custom_blend;
        blender.fade_to(current_animation, blend_time);
    }
    else {
        blender.play(current_animation);
        playback = 0.0f;
    }

    speed = custom_speed;
    playing = true;
    current_animation_name = animation_name;

    // sequence with default values
    timeline.frame_max = current_animation->get_track(0)->size();;
    selected_track = -1;

    generate_track_data();
}

void AnimationPlayer::generate_track_data()
{
    timeline.tracks.clear();
    track_data.clear();
    uint32_t num_tracks = current_animation->get_track_count();
    track_data.resize(num_tracks);
    
    // Generate data from tracks
    for (uint32_t i = 0; i < num_tracks; ++i) {

        const std::string& track_path = current_animation->get_track(i)->get_path();
   
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

        generate_track_timeline_data(i, track_path);       
    }
}

void AnimationPlayer::generate_track_timeline_data(uint32_t track_idx, const std::string& track_path)
{
    int count = 1;
    int type = -1;

    Track* track = current_animation->get_track(track_idx);

    if (track->get_type() == TrackType::TYPE_POSITION) {
        type = 0;
        count = 3;
    }
    else if (track->get_type() == TrackType::TYPE_ROTATION) {
        type = 1;
        count = 4;
    }
    else if (track->get_type() == TrackType::TYPE_SCALE) {
        type = 2;
        count = 3;
    }
    const int c = count;

    std::vector<ImVec2>* points = new std::vector<ImVec2>[count];
    std::vector<bool> edited_points;
    //timeline.curve_editor.SetPointsCount(animation->get_track(i)->size());
    std::vector<uint32_t> keyposes = current_animation->get_keyposes();
    if (keyposes.size())
    {
        for (uint32_t j = 0; j < keyposes.size(); j++)
        {
            if (type == 0 || type == 2) {
                glm::vec3 p = std::get<glm::vec3>(track->get_keyframe(keyposes[j]).value);

                points[0].push_back(ImVec2(keyposes[j], p.x));
                points[1].push_back(ImVec2(keyposes[j], p.y));
                points[2].push_back(ImVec2(keyposes[j], p.z));

                /*timeline.curve_editor.point_count[0] = points[0].size();
                timeline.curve_editor.point_count[1] = points[1].size();
                timeline.curve_editor.point_count[2] = points[2].size();*/
            }
            else if (type == 1) {
                glm::quat p = std::get<glm::quat>(track->get_keyframe(keyposes[j]).value);
                points[0].push_back(ImVec2(keyposes[j], p.x));
                points[1].push_back(ImVec2(keyposes[j], p.y));
                points[2].push_back(ImVec2(keyposes[j], p.z));
                points[3].push_back(ImVec2(keyposes[j], p.w));

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
    }
    else
    {
        for (uint32_t j = 0; j < track->size(); j++) {
            if (type == 0 || type == 2) {
                glm::vec3 p = std::get<glm::vec3>(track->get_keyframe(j).value);

                points[0].push_back(ImVec2(j, p.x));
                points[1].push_back(ImVec2(j, p.y));
                points[2].push_back(ImVec2(j, p.z));

                /*timeline.curve_editor.point_count[0] = points[0].size();
                timeline.curve_editor.point_count[1] = points[1].size();
                timeline.curve_editor.point_count[2] = points[2].size();*/
            }
            else if (type == 1) {
                glm::quat p = std::get<glm::quat>(track->get_keyframe(j).value);
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
    }
    timeline.tracks.push_back(Timeline::TimelineTrack{ track_idx, track->get_type(), 0, (int)track->size(), false, track_path, points, edited_points });
    timeline.tracks[timeline.tracks.size() - 1].points = new std::vector<ImVec2>[count];
    timeline.tracks[timeline.tracks.size() - 1].points = points;
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
    current_animation = blender.get_current_animation();
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

float stretch = 5.f;
float window = 2.f;
float sigma = 0.5f;
int min_f = 10;
glm::vec3 direction(1.f, 0.f, 0.f);

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

    int current_frame = playback * (timeline.frame_max - timeline.frame_min) / current_animation->get_duration();
 
    ImGui::DragFloat("Stretchiness", &stretch, 0.1f, 0.f, 100.f);
    ImGui::DragFloat("Window frames", &window, 1.f, 1.f, 100.f);
    ImGui::DragFloat("Sigma", &sigma, 0.1f, 0.f, 100.f);
    ImGui::DragInt("Min frames", &min_f, 1, 0, current_animation->get_track(0)->size() - 1);
    int v[3] = { direction.x, direction.y, direction.z };
    if (ImGui::InputInt3("Direction", v))
    {
        direction = glm::vec3(v[0], v[1], v[2]);
    }
    if (current_animation->get_type() == ANIM_TYPE_SKELETON)
    {
        if (ImGui::Button("Generate keyposes") && !playing)
        {
            current_animation->compute_keyposes(current_frame, stretch, window, min_f, sigma, direction);
            timeline.tracks.clear();
            for (size_t i = 0; i < current_animation->get_track_count(); i++)
            {
                const std::string& track_path = current_animation->get_track(i)->get_path();
                generate_track_timeline_data(i, track_path);
            }
        }
    }

    // Timeline
    if (ImGui::Begin("Timeline")) {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y / 3));
        ImGui::SetWindowPos(ImVec2(0, io.DisplaySize.y - io.DisplaySize.y / 3));

        // let's create the sequencer
        static int selected_entry = selected_track;
        static int first_frame = 0;
        static bool expanded = true;
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
        if (selected_entry != -1 && timeline.keyframe_selection_changed)
        {            

            const Timeline::TimelineTrack item = timeline.tracks[timeline.selected_point.x];

            // update current frame on select frame
            if (selected_frame != timeline.selected_point.y)
            {
                new_current_frame = item.points[0][timeline.selected_point.y].x;
                timeline.keyframe_selection_changed = false;
            }

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
            //timeline.selected_point.y = -1;
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

    current_animation = blender.get_current_animation();

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
