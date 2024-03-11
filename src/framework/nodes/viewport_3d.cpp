#include "viewport_3d.h"

#include "graphics/renderer.h"

Viewport3D::Viewport3D(Node2D* root_2d) : Node3D(), root(root_2d)
{
    root->remove_flag(MATERIAL_2D);
}

Viewport3D::~Viewport3D()
{
   
}

// Screen space
// UI 1000x1000
// Pss: 200, 200, 0
// Sss: 200, 200, 0
// Pws: 0.2 * VW, 0.2 * VH, 0
// Sws: 0.2 * VW, 0.2 * VH, 0
// model

void Viewport3D::render()
{
    root->render();
}

void Viewport3D::update(float delta_time)
{
    root->update(delta_time);

    glm::vec2 pos_2d = root->get_translation();

    auto webgpu_context = Renderer::instance->get_webgpu_context();

    float width = webgpu_context->render_width;
    float height = webgpu_context->render_height;
    glm::vec2 screen_size(width, height);

    pos_2d /= screen_size;

    pos_2d *= viewport_size;

    // root->set_translation(pos_2d);
    //root->scale(glm::vec2(0.0001));
}
