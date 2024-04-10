#include "spot_light_3d.h"
#include "graphics/renderer.h"
#include "imgui.h"

SpotLight3D::SpotLight3D() : Light3D()
{
    type = LIGHT_SPOT;
}

SpotLight3D::~SpotLight3D()
{
    
}

void SpotLight3D::render_gui()
{
    bool changed = false;

    if (ImGui::TreeNodeEx("SpotLight3D"))
    {
        changed |= ImGui::SliderFloat("Range", &range, 0.f, 10.0f);
        changed |= ImGui::SliderFloat("Inner Cos", &angle, 0.f, 360.0f);
        changed |= ImGui::SliderFloat("Outer Cos", &angle, 0.f, 360.0f);

        if (changed)
        {
            Renderer::instance->update_lights();
        }

        ImGui::TreePop();
    }

    Light3D::render_gui();
}

sLightUniformData SpotLight3D::get_uniform_data()
{
    return {
        .position = get_translation(),
        .type = type,
        .color = color,
        .intensity = intensity,
        .range = range,
        //  todo
        .inner_cone_cos = 0.0f,
        .outer_cone_cos = 0.0f
    };
}

void SpotLight3D::set_range(float value)
{
    this->range = value;
}

void SpotLight3D::set_angle(float value)
{
    this->angle = value;
}
