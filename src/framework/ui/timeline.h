#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include "framework/utils/ImSequencer.h"
#include "framework/utils/ImCurveEdit.h"

#include "framework/animation/track.h"

#include "iostream"
#include <algorithm>
#include <vector>

static const char* TrackTypes[] = {
    "UNDEFINED",
    "FLOAT", // Set a value in a property, can be interpolated.
    "VECTOR3", // Vector3 track, can be compressed.
    "QUAT", // Quaternion track, can be compressed.
    "METHOD", // Call any method on a specific node.
    "POSITION",
    "ROTATION",
    "SCALE" };

struct CurveEdit : public ImCurveEdit::Delegate
{
    CurveEdit()
    {
        /*points[0][0] = ImVec2(-10.f, 0);
        points[0][1] = ImVec2(20.f, 0.6f);
        points[0][2] = ImVec2(25.f, 0.2f);
        points[0][3] = ImVec2(70.f, 0.4f);
        points[0][4] = ImVec2(120.f, 1.f);
        mPointCount[0] = 5;

        points[1][0] = ImVec2(-50.f, 0.2f);
        points[1][1] = ImVec2(33.f, 0.7f);
        points[1][2] = ImVec2(80.f, 0.2f);
        points[1][3] = ImVec2(82.f, 0.8f);
        mPointCount[1] = 4;


        points[2][0] = ImVec2(40.f, 0);
        points[2][1] = ImVec2(60.f, 0.1f);
        points[2][2] = ImVec2(90.f, 0.82f);
        points[2][3] = ImVec2(150.f, 0.24f);
        points[2][4] = ImVec2(200.f, 0.34f);
        points[2][5] = ImVec2(250.f, 0.12f);
        mPointCount[2] = 6;
        is_visible[0] = is_visible[1] = is_visible[2] = true;
        max = ImVec2(1.f, 1.f);
        min = ImVec2(0.f, 0.f);*/
    }
    size_t GetCurveCount()
    {
        return 4;
    }

    bool IsVisible(size_t curveIndex)
    {
        return is_visible[curveIndex];
    }
    size_t GetPointCount(size_t curveIndex)
    {
        return point_count[curveIndex];
    }

    uint32_t GetCurveColor(size_t curveIndex)
    {
        uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
        return cols[curveIndex];
    }
    ImVec2* GetPoints(size_t curveIndex)
    {
        return points[curveIndex];
    }
    virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const { return ImCurveEdit::CurveSmooth; }
    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        points[curveIndex][pointIndex] = ImVec2(value.x, value.y);
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (points[curveIndex][i].x == value.x)
                return (int)i;
        }
        return pointIndex;
    }
    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        if (point_count[curveIndex] >= max_point_count)
            return;

        points[curveIndex][point_count[curveIndex]++] = value;
        SortValues(curveIndex);
    }
    virtual ImVec2& GetMax() { return max; }
    virtual ImVec2& GetMin() { return min; }
    virtual unsigned int GetBackgroundColor() { return 0; }

    void SetPointsCount(size_t count) {
        max_point_count = count;

        for (size_t i = 0; i < 4; i++) {
            points[i] = new ImVec2[count];
            point_count[i] = count;
        }
    }
    size_t max_point_count = 200;
    size_t point_count[4];
    ImVec2* points[4];
    bool is_visible[4];
    ImVec2 min;
    ImVec2 max;

private:
    void SortValues(size_t curveIndex)
    {
       /* auto b = std::begin(points[curveIndex]);
        auto e = std::begin(points[curveIndex]) + GetPointCount(curveIndex);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });*/

    }
};

struct Timeline : public ImSequencer::SequenceInterface
{
    // interface with sequencer

    virtual int GetFrameMin() const {
        return frame_min;
    }
    virtual int GetFrameMax() const {
        return frame_max;
    }
    virtual int GetItemCount() const { return (int)tracks.size(); }

    virtual int GetItemTypeCount() const { return sizeof(TrackTypes) / sizeof(char*); }
    virtual const char* GetItemTypeName(int typeIndex) const { return TrackTypes[typeIndex]; }
    virtual const char* GetItemLabel(int index) const
    {
        static char tmps[512];
        snprintf(tmps, 512, "[%02d] %s", index, tracks[index].name.c_str());
        return tmps;
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
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
    virtual void Add(int type) { tracks.push_back(TimelineTrack{ type, 0, 10, false, "no-name"}); };
    virtual void Del(int index) { tracks.erase(tracks.begin() + index); }
    virtual void Duplicate(int index) { tracks.push_back(tracks[index]); }

    virtual size_t GetCustomHeight(int index) { return tracks[index].expanded ? 300 : 0; }

    // my datas
    Timeline() : frame_min(0), frame_max(1000) {}
    int frame_min, frame_max;
    ImVec2 selected_point = { -1, -1 };

    struct TimelineTrack
    {
        int type;
        int frame_start, frame_end;
        bool expanded;
        std::string name;
        std::vector<ImVec2>* points;
    };

    std::vector<TimelineTrack> tracks;
    CurveEdit curve_editor;

    virtual void DoubleClick(int index) {
        if (tracks[index].expanded)
        {
            tracks[index].expanded = false;
            return;
        }
        for (auto& item : tracks)
            item.expanded = false;

        std::vector<ImVec2>* points = tracks[index].points;

        int count = 3;
        if (TrackTypes[tracks[index].type] == "ROTATION")
            count = 4;
        else if (TrackTypes[tracks[index].type] == "FLOAT")
            count = 1;

        /*for (size_t i = 0; i < count; i++) {
            for (size_t j = 0; j < points[i].size(); j++) {
                curve_editor.AddPoint(i, points[i][j]);
            }
        }*/
        tracks[index].expanded = !tracks[index].expanded;
    }

    int DrawPoint(ImDrawList* draw_list, ImVec2 pos, const ImVec2 size, const ImVec2 offset, int type, bool edited, bool selected)
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
        case TrackType::TYPE_POSITION:
            fill_color = ImGui::GetColorU32(ImVec4(0.30f, 0.8f, 0.64f, 1.f));//0xE94560DD;//0xFFE94560;
            break;

        case TrackType::TYPE_ROTATION:
            fill_color = ImGui::GetColorU32(ImVec4(0.97f, 0.27f, 0.37f, 1.f));//0xAAFFD700;
            break;
        case TrackType::TYPE_SCALE:
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
            draw_list->AddPolyline(offsets, 4, 0xFFFFFFFF, true, 3.0f);
        else if (ret || selected)
            draw_list->AddPolyline(offsets, 4, 0x8080B0FF, true, 2.0f);
        else
            draw_list->AddPolyline(offsets, 4, 0x00008080, true, 2.0f);

        return ret;
    }
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
        for (int j = tracks[index].frame_start; j < tracks[index].frame_end; j++) {
            //const int drawState = DrawPoint(draw_list, ImVec2(j, rc.Min.y ), ImVec2(5, 5), ImVec2(5, 5), false);
        }
        /*static const char* labels[] = { "x", "y" , "z", "w"};

        curve_editor.max = ImVec2(float(frame_max), 1.f);
        curve_editor.min = ImVec2(float(frame_min), 0.f);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);

        int count = 3;
        if (TrackTypes[tracks[index].type] == "ROTATION")
            count = 4;
        else if (TrackTypes[tracks[index].type] == "FLOAT")
            count = 1;

        for (int i = 0; i < count; i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            draw_list->AddText(pta, curve_editor.is_visible[i] ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                curve_editor.is_visible[i] = !curve_editor.is_visible[i];
        }

        for (int j = tracks[index].frame_start; j < tracks[index].frame_end; j++) {
            const int drawState = DrawPoint(draw_list, ImVec2(j, rc.Min.y), ImVec2(5,5), ImVec2(5, 5), false);
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);*/
        //ImCurveEdit::Edit(curve_editor, ImVec2(rc.Max.x - rc.Min.x, rc.Max.y - rc.Min.y), 137 + index, &clippingRect);
    }

    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
    {
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        for (int j = tracks[index].frame_start; j < tracks[index].frame_end; j++) {
            float r = (j - frame_min) / float(frame_max - frame_min);
            float x = ImLerp(rc.Min.x, rc.Max.x, r);
            bool selected = false;
            if (selected_point.x == index && selected_point.y == j) {
                selected = true;
            }
            //draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
            const int drawState = DrawPoint(draw_list, ImVec2(x, rc.Min.y), ImVec2(1,1), ImVec2(0, 10), tracks[index].type, false, selected);
            if (drawState == 2) {
                selected_point = ImVec2(index, j);
                std::cout << "Frame " << j << "selected: " << tracks[index].name << std::endl;
            }
        }
        draw_list->PopClipRect(); 
        /*curve_editor.max = ImVec2(float(frame_max), 1.f);
        curve_editor.min = ImVec2(float(frame_min), 0.f);
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);

        int count = 3;
        if (TrackTypes[tracks[index].type] == "ROTATION")
            count = 4;
        else if (TrackTypes[tracks[index].type] == "FLOAT")
            count = 1;

        ImGuiIO& io = ImGui::GetIO();
        static bool scrollingV = false;
        const ImVec2 size = ImVec2(rc.Max.x - rc.Min.x, rc.Max.y - rc.Min.y);
        const ImVec2 offset = ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + size.y);

        for (int j = tracks[index].frame_start; j < tracks[index].frame_end; j++) {
            const int drawState = DrawPoint(draw_list, ImVec2(j, rc.Min.y), size, ImVec2(5,5), false);
        }

        for (int i = 0; i < count; i++)
        {
            for (int j = 0; j < curve_editor.point_count[i]; j++)
            {
                float p = curve_editor.points[i][j].x;
                if (p < tracks[index].frame_start || p > tracks[index].frame_end)
                    continue;
                float r = (p - frame_min) / float(frame_max - frame_min);
                float x = ImLerp(rc.Min.x, rc.Max.x, r);
                draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
            }
        }
        draw_list->PopClipRect();*/
    }
};
