#include "viewport_3d.h"

#include "graphics/renderer.h"
#include "framework/scene/parse_scene.h"
#include "framework/input.h"

Viewport3D::Viewport3D(Node2D* root_2d) : Node3D(), root(root_2d)
{
    root->remove_flag(MATERIAL_2D);

    raycast_pointer = parse_mesh("data/meshes/raycast.obj");

    Material pointer_material;
    pointer_material.shader = RendererStorage::get_shader("data/shaders/ui/ui_ray_pointer.wgsl", pointer_material);

    raycast_pointer->set_surface_material_override(raycast_pointer->get_surface(0), pointer_material);
}

Viewport3D::~Viewport3D()
{
   
}

void Viewport3D::render()
{
    root->render();

    raycast_pointer->render();
}

void Viewport3D::update(float delta_time)
{
    if(Renderer::instance->get_openxr_available())
    {
        glm::mat4x4 raycast_transform = Input::get_controller_pose(HAND_RIGHT, POSE_AIM);
        raycast_pointer->set_model(raycast_transform);
    }

    glm::vec2 pos_2d = root->get_translation();

    auto webgpu_context = Renderer::instance->get_webgpu_context();

    float width = webgpu_context->render_width;
    float height = webgpu_context->render_height;
    float ar = width / height;

    glm::vec2 screen_size(width, height);
    pos_2d /= screen_size;

    root->set_translation(pos_2d);
    root->scale(1.0f / glm::vec2(width, height * ar));

    // TODO; apply transform of the viewport3d node itself..
    // ...

    root->update(delta_time);
}
