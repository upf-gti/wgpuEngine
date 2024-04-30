#pragma once

#include "node_3d.h"
#include "../animation/blend_animation.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <vector>

#include "framework/utils/ImSequencer.h"
#include "framework/utils/ImCurveEdit.h"

class MeshInstance3D;

enum BlendType {
    CROSSFADE,
    ADDITIVE
};

static const char* SequencerItemTypeNames[] = {
    "UNDEFINED",
    "FLOAT", // Set a value in a property, can be interpolated.
    "VECTOR3", // Vector3 track, can be compressed.
    "QUAT", // Quaternion track, can be compressed.
    "METHOD", // Call any method on a specific node.
    "POSITION",
    "ROTATION",
    "SCALE" };

 struct RampEdit : public ImCurveEdit::Delegate
  {
      RampEdit()
      {
          /*mPts[0][0] = ImVec2(-10.f, 0);
          mPts[0][1] = ImVec2(20.f, 0.6f);
          mPts[0][2] = ImVec2(25.f, 0.2f);
          mPts[0][3] = ImVec2(70.f, 0.4f);
          mPts[0][4] = ImVec2(120.f, 1.f);
          mPointCount[0] = 5;

          mPts[1][0] = ImVec2(-50.f, 0.2f);
          mPts[1][1] = ImVec2(33.f, 0.7f);
          mPts[1][2] = ImVec2(80.f, 0.2f);
          mPts[1][3] = ImVec2(82.f, 0.8f);
          mPointCount[1] = 4;


          mPts[2][0] = ImVec2(40.f, 0);
          mPts[2][1] = ImVec2(60.f, 0.1f);
          mPts[2][2] = ImVec2(90.f, 0.82f);
          mPts[2][3] = ImVec2(150.f, 0.24f);
          mPts[2][4] = ImVec2(200.f, 0.34f);
          mPts[2][5] = ImVec2(250.f, 0.12f);
          mPointCount[2] = 6;
          mbVisible[0] = mbVisible[1] = mbVisible[2] = true;
          mMax = ImVec2(1.f, 1.f);
          mMin = ImVec2(0.f, 0.f);*/
      }
      size_t GetCurveCount()
      {
          return 4;
      }

      bool IsVisible(size_t curveIndex)
      {
          return mbVisible[curveIndex];
      }
      size_t GetPointCount(size_t curveIndex)
      {
          return mPointCount[curveIndex];
      }

      uint32_t GetCurveColor(size_t curveIndex)
      {
          uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
          return cols[curveIndex];
      }
      ImVec2* GetPoints(size_t curveIndex)
      {
          return mPts[curveIndex];
      }
      virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const { return ImCurveEdit::CurveSmooth; }
      virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
      {
          mPts[curveIndex][pointIndex] = ImVec2(value.x, value.y);
          SortValues(curveIndex);
          for (size_t i = 0; i < GetPointCount(curveIndex); i++)
          {
              if (mPts[curveIndex][i].x == value.x)
                  return (int)i;
          }
          return pointIndex;
      }
      virtual void AddPoint(size_t curveIndex, ImVec2 value)
      {
          if (mPointCount[curveIndex] >= mCount)
              return;
          mPts[curveIndex][mPointCount[curveIndex]++] = value;
          SortValues(curveIndex);
      }
      virtual ImVec2& GetMax() { return mMax; }
      virtual ImVec2& GetMin() { return mMin; }
      virtual unsigned int GetBackgroundColor() { return 0; }
      virtual void SetCountPoints(unsigned int count) {
          mCount = count;
          mPts[4][count];
      }
      unsigned int mCount = 8;
      ImVec2 mPts[4][8];
      size_t mPointCount[4];
      bool mbVisible[4];
      ImVec2 mMin;
      ImVec2 mMax;

  private:
      void SortValues(size_t curveIndex)
      {
          auto b = std::begin(mPts[curveIndex]);
          auto e = std::begin(mPts[curveIndex]) + GetPointCount(curveIndex);
          std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });

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

    virtual int GetItemTypeCount() const { return sizeof(SequencerItemTypeNames) / sizeof(char*); }
    virtual const char* GetItemTypeName(int typeIndex) const { return SequencerItemTypeNames[typeIndex]; }
    virtual const char* GetItemLabel(int index) const
    {
        static char tmps[512];
        snprintf(tmps, 512, "[%02d] %s", index, SequencerItemTypeNames[tracks[index].type]);
        return tmps;
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
    {
        TimelineTrack& item = tracks[index];
        if (color)
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        if (start)
            *start = &item.frame_start;
        if (end)
            *end = &item.frame_end;
        if (type)
            *type = item.type;
    }
    virtual void Add(int type) { tracks.push_back(TimelineTrack{ type, 0, 10, false }); };
    virtual void Del(int index) { tracks.erase(tracks.begin() + index); }
    virtual void Duplicate(int index) { tracks.push_back(tracks[index]); }

    virtual size_t GetCustomHeight(int index) { return tracks[index].expanded ? 300 : 0; }

    // my datas
    Timeline() : frame_min(0), frame_max(0) {}
    int frame_min, frame_max;
    struct TimelineTrack
    {
        int type;
        int frame_start, frame_end;
        bool expanded;
        std::string name;
    };
    std::vector<TimelineTrack> tracks;
    RampEdit rampEdit;

    virtual void DoubleClick(int index) {
        if (tracks[index].expanded)
        {
            tracks[index].expanded = false;
            return;
        }
        for (auto& item : tracks)
            item.expanded = false;
        tracks[index].expanded = !tracks[index].expanded;
    }
    
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
        static const char* labels[] = { "Translation", "Rotation" , "Scale", "Other"};

        rampEdit.mMax = ImVec2(float(frame_max), 1.f);
        rampEdit.mMin = ImVec2(float(frame_min), 0.f);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < 4; i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            draw_list->AddText(pta, rampEdit.mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                rampEdit.mbVisible[i] = !rampEdit.mbVisible[i];
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(rampEdit, ImVec2(rc.Max.x - rc.Min.x, rc.Max.y - rc.Min.y), 137 + index, &clippingRect);
    }

    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
    {
        rampEdit.mMax = ImVec2(float(frame_max), 1.f);
        rampEdit.mMin = ImVec2(float(frame_min), 0.f);
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < rampEdit.mPointCount[i]; j++)
            {
                float p = rampEdit.mPts[i][j].x;
                if (p < tracks[index].frame_start || p > tracks[index].frame_end)
                    continue;
                float r = (p - frame_min) / float(frame_max - frame_min);
                float x = ImLerp(rc.Min.x, rc.Max.x, r);
                draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
            }
        }
        draw_list->PopClipRect();
    }
};


class AnimationPlayer : public Node3D
{
    Node3D* root_node = nullptr;

    std::string current_animation_name;

    bool autoplay   = false;
    bool playing    = false;
    bool looping    = true;
    bool blend_type = BlendType::CROSSFADE;

    float playback   = 0.f;
    float speed      = 1.f;
    float blend_time = 0.f;

    BlendAnimation blender;
    Timeline timeline;

    std::vector<void*> track_data;

    void generate_track_data();

public:

    AnimationPlayer(const std::string& name);

    void play(const std::string& animation_name = "", float custom_blend = -1, float custom_speed = 1.0, bool from_end = false);
    void pause();
    void stop(bool keep_state = false);

    void update(float delta_time) override;
    void render_gui() override;

    void set_speed(float time);
    void set_blend_time(float time);
    void set_looping(bool loop);
    void set_root_node(Node3D* new_root_node);

    float get_blend_time();
    float get_speed();
    bool is_playing();
    bool is_looping();
};

