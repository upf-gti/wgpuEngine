#include "timeline.h"

    int Timeline::GetFrameMin() const {
        return frame_min;
    }
    int Timeline::GetFrameMax() const {
        return frame_max;
    }
    int Timeline::GetItemCount() const { return (int)tracks.size(); }

    int Timeline::GetItemTypeCount() const { return sizeof(TrackTypes) / sizeof(char*); }
    const char* Timeline::GetItemTypeName(int typeIndex) const { return TrackTypes[typeIndex]; }
    const char* Timeline::GetItemLabel(int index) const
    {
        static char tmps[512];
        snprintf(tmps, 512, "[%02d] %s", index, tracks[index].name.c_str());
        return tmps;
    }

    void Timeline::Get(int index, int** start, int** end, int* type, unsigned int* color)
    {
        TimelineTrack& item = tracks[index];
        if (color)
            *color = 0xAA1A1A1A;//0xAAAA8080; // same color for everyone, return color based on type
        if (start)
            *start = &item.frame_start;
        if (end)
            *end = &item.frame_end;
        if (type)
            *type = item.type;
    }
    void Timeline::Add(int type) { tracks.push_back(TimelineTrack{ (uint32_t)tracks.size(), type, 0, 10, false, "no-name" }); };
    void Timeline::Del(int index) { tracks.erase(tracks.begin() + index); }
    void Timeline::Duplicate(int index) { tracks.push_back(tracks[index]); }

    size_t Timeline::GetCustomHeight(int index) { return tracks[index].expanded ? 300 : 0; }

    void Timeline::DoubleClick(int index) {
        if (tracks[index].expanded)
        {
            tracks[index].expanded = false;
            return;
        }
        for (auto& item : tracks)
            item.expanded = false;

        int count = 3;
        if (TrackTypes[tracks[index].type] == "ROTATION")
            count = 4;
        else if (TrackTypes[tracks[index].type] == "FLOAT")
            count = 1;

        tracks[index].expanded = !tracks[index].expanded;
    }

    int Timeline::DrawPoint(ImDrawList* draw_list, ImVec2 pos, const ImVec2 size, const ImVec2 offset, int type, bool edited, bool selected)
    {
        int ret = 0;
        ImGuiIO& io = ImGui::GetIO();

        static const ImVec2 localOffsets[4] = { ImVec2(1,0), ImVec2(0,1), ImVec2(-1,0), ImVec2(0,-1) };
        ImVec2 offsets[4];
        for (int i = 0; i < 4; i++)
        {
            //pos * size + localOffsets[i] * 4.5f + offset;
            offsets[i] = ImVec2(pos.x * size.x + localOffsets[i].x * 4.5f + offset.x, pos.y * size.y + localOffsets[i].y * 4.5f + offset.y);
        }

        const ImVec2 center = ImVec2(pos.x * size.x + offset.x, pos.y * size.y + offset.y);
        const ImRect anchor(ImVec2(center.x - 5, center.y - 5), ImVec2(center.x + 5, center.y + 5));
        ImColor fill_color = 0xAA000000;

        switch (type) {
        case eTrackType::TYPE_POSITION:
            fill_color = ImGui::GetColorU32(ImVec4(0.30f, 0.8f, 0.64f, 1.f));//0xE94560DD;//0xFFE94560;
            break;

        case eTrackType::TYPE_ROTATION:
            fill_color = ImGui::GetColorU32(ImVec4(0.97f, 0.27f, 0.37f, 1.f));//0xAAFFD700;
            break;
        case eTrackType::TYPE_SCALE:
            fill_color = ImGui::GetColorU32(ImVec4(1.f, 0.84f, 0.f, 1.f));// 0xFFD700AA;// 0xFFFF5ADD;
            break;
        }

        draw_list->AddConvexPolyFilled(offsets, 4, fill_color);

        if (anchor.Contains(io.MousePos))
        {
            ret = 1;
            if (io.MouseDown[0]) {
                ret = 2;
            }
        }
        if (edited)
            draw_list->AddPolyline(offsets, 4, 0xFFFFFFFF, true, 2.0f);
        else if (ret || selected)
            draw_list->AddPolyline(offsets, 4, 0x8080B0FF, true, 3.0f);
        else
            draw_list->AddPolyline(offsets, 4, 0x00008080, true, 2.0f);

        return ret;
    }

    void Timeline::CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
    {
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        int count = 3;
        if (TrackTypes[tracks[index].type] == "ROTATION")
            count = 4;
        else if (TrackTypes[tracks[index].type] == "FLOAT")
            count = 1;

        //for (int j = tracks[index].frame_start; j < tracks[index].frame_end; j++) {
        for (int p = 0; p < tracks[index].points->size(); p++) {
            int j = tracks[index].points[0][p].x;
            float r = (j - frame_min) / float(frame_max - frame_min);
            float x = ImLerp(rc.Min.x, rc.Max.x, r);
            bool selected = false;

            if (selected_point.x == index && selected_point.y == p) {
                selected = true;
            }
            const int drawState = DrawPoint(draw_list, ImVec2(x, rc.Min.y), ImVec2(1, 1), ImVec2(0, 10), tracks[index].type, tracks[index].edited_points[p], selected);

            if (drawState == 2) {
                selected_point = ImVec2(index, p);
                keyframe_selection_changed = true;
                std::cout << "Frame " << p << "selected: " << tracks[index].name << std::endl;
            }
        }
        draw_list->PopClipRect();       
    }

