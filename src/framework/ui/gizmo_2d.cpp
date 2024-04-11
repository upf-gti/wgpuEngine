#include "gizmo_2d.h"

#include "graphics/renderer.h"
#include "framework/input.h"
#include "framework/camera/camera.h"

void Gizmo2D::set_mode(ImGuizmo::MODE new_mode)
{
    this->mode = new_mode;
}

void Gizmo2D::set_operation(ImGuizmo::OPERATION new_operation)
{
    this->operation = new_operation;
}

void Gizmo2D::render(const glm::mat4x4& m_view, const glm::mat4x4& m_proj, glm::mat4x4& m_model)
{
    if (Input::was_key_pressed(GLFW_KEY_1))
        operation = ImGuizmo::TRANSLATE;
    if (Input::was_key_pressed(GLFW_KEY_2))
        operation = ImGuizmo::ROTATE;
    if (Input::was_key_pressed(GLFW_KEY_3))
        operation = ImGuizmo::SCALE;

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(glm::value_ptr(m_view), glm::value_ptr(m_proj),
        operation, mode, glm::value_ptr(m_model));
}
