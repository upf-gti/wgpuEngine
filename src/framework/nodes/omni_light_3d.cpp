#include "omni_light_3d.h"
#include "graphics/renderer.h"
#include "imgui.h"

OmniLight3D::OmniLight3D() : Light3D()
{
    type = LIGHT_OMNI;
}

OmniLight3D::~OmniLight3D()
{

}

void OmniLight3D::render_gui()
{
    bool changed = false;

    if (ImGui::TreeNodeEx("OmniLight3D"))
    {
        changed |= ImGui::SliderFloat("Range", &range, 0.f, 10.0f);

        if (changed)
        {
            Renderer::instance->update_lights();
        }

        ImGui::TreePop();
    }

    Light3D::render_gui();
}

sLightUniformData OmniLight3D::get_uniform_data()
{
    return {
        .position = get_translation(),
        .type = type,
        .color = color,
        .intensity = intensity,
        .range = range
    };
}

void OmniLight3D::set_range(float value)
{
    this->range = value;
}
