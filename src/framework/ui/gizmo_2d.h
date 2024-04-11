#pragma once

#include "framework/math.h"

#include "imgui.h"
#include "framework/utils/ImGuizmo.h"

class Gizmo2D {

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE mode = ImGuizmo::WORLD;

public:

    void set_mode(ImGuizmo::MODE new_mode);
    void set_operation(ImGuizmo::OPERATION new_operation);

    void render(const glm::mat4x4& m_view, const glm::mat4x4& m_proj, glm::mat4x4& m_model);
};
