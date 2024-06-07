#include "io.h"

#include "framework/nodes/ui.h"
#include "framework/scene/parse_scene.h"
#include "framework/input.h"

#include "shaders/ui/ui_ray_pointer.wgsl.gen.h"

float IO::xr_ray_distance = 0.0f;

Node2D* IO::focused = nullptr;
Node2D* IO::hovered = nullptr;

MeshInstance3D* IO::ray_pointer = nullptr;

glm::vec2 IO::xr_position = { 0.0f, 0.0f };
glm::vec3 IO::xr_world_position = { 0.0f, 0.0f, 0.0f };

void IO::initialize()
{
    // Controller pointer
    {
        ray_pointer = parse_mesh("data/meshes/raycast.obj");

        Material pointer_material;
        pointer_material.transparency_type = ALPHA_BLEND;
        pointer_material.cull_type = CULL_NONE;
        pointer_material.shader = RendererStorage::get_shader_from_source(shaders::ui_ray_pointer::source, shaders::ui_ray_pointer::path, pointer_material);

        ray_pointer->set_surface_material_override(ray_pointer->get_surface(0), pointer_material);
    }
}

void IO::start_frame()
{
    set_xr_ray_distance(-1.0f);
}

// Call this pre-render!!
void IO::end_frame()
{
    Node2D::process_input();

    const glm::mat4x4& raycast_transform = Input::get_controller_pose(HAND_RIGHT, POSE_AIM);
    ray_pointer->set_model(raycast_transform);
    ray_pointer->scale(glm::vec3(1.0f, 1.0f, xr_ray_distance < 0.0f ? 0.5f : xr_ray_distance));
}

void IO::set_focus(Node2D* node)
{
    focused = node;
}

void IO::set_hover(Node2D* node, const glm::vec2& p, float ray_distance)
{
    hovered = node;

    xr_ray_distance = ray_distance;

    set_xr_position(p);
}

bool IO::is_hover_disabled()
{
    if (!hovered) {
        return false;
    }

    ui::Button2D* button = dynamic_cast<ui::Button2D*>(hovered);
    if (!button) {
        return false;
    }

    return button->disabled;
}

bool IO::is_focus_type(uint32_t type)
{
    if (!any_focus()) {
        return false;
    }

    return (focused->get_class_type() == type);
}

bool IO::equals_focus(Node2D* node)
{
    return (focused == node);
}

bool IO::any_focus()
{
    return (focused != nullptr);
}

bool IO::any_hover()
{
    return (hovered != nullptr);
}
