#include "spot_light_3d.h"
#include "graphics/renderer.h"
#include "framework/utils/utils.h"
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
    constexpr float pi_2 = glm::pi<float>() * 0.5f;

    if (ImGui::TreeNodeEx("SpotLight3D"))
    {
        changed |= ImGui::SliderFloat("Range", &range, 0.f, 10.0f);
        changed |= ImGui::SliderFloat("Inner Angle", &inner_cone_angle, 0.f, pi_2);
        changed |= ImGui::SliderFloat("Outer Angle", &outer_cone_angle, 0.f, pi_2);

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
        .direction = -get_global_model()[2],
        .range = range,
        .inner_cone_cos = cosf(inner_cone_angle),
        .outer_cone_cos = cosf(outer_cone_angle)
    };
}

void SpotLight3D::set_range(float value)
{
    this->range = value;
}

void SpotLight3D::set_inner_cone_angle(float value)
{
    this->inner_cone_angle = value;
}

void SpotLight3D::set_outer_cone_angle(float value)
{
    this->outer_cone_angle = value;
}
