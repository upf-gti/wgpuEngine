#include "viewport_3d.h"

#include "graphics/renderer.h"

#include "framework/nodes/node_2d.h"
#include "framework/parsers/parse_scene.h"
#include "framework/input.h"

Viewport3D::Viewport3D(Node2D* root_2d) : Node3D(), root(root_2d)
{
    root->disable_2d();
}

Viewport3D::~Viewport3D()
{
   
}

void Viewport3D::set_viewport_size(const glm::vec2& new_size)
{
    viewport_size = new_size;
}

void Viewport3D::render()
{
    if (!active) {
        return;
    }

    root->render();
}

void Viewport3D::update(float delta_time)
{
    if (!active) {
        return;
    }

    // Manage 3d transform data

    glm::vec2 pos_2d = root->get_translation();

    auto webgpu_context = Renderer::instance->get_webgpu_context();

    float width = static_cast<float>(webgpu_context->render_width);
    float height = static_cast<float>(webgpu_context->render_height);
    float ar = width / height;

    glm::vec2 screen_size(width, height);
    pos_2d /= screen_size;

    root->set_position(pos_2d);
    root->scale(1.0f / glm::vec2(width, height * ar));

    root->set_viewport_model(get_global_model());

    root->update(delta_time);
}
