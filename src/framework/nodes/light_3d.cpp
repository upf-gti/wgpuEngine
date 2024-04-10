#include "light_3d.h"
#include "graphics/renderer.h"
#include "imgui.h"

Light3D::Light3D()
{
}

Light3D::~Light3D()
{
    
}

void Light3D::render_gui()
{
    if (ImGui::TreeNodeEx("Light3D", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool changed = false;

        changed |= ImGui::ColorEdit3("Color", &color[0]);
        changed |= ImGui::SliderFloat("Intensity", &intensity, 0.f, 10.0f);
        changed |= ImGui::Checkbox("Cast Shadows", &cast_shadows);

        if (cast_shadows) {
            changed |= ImGui::SliderFloat("Shadow Bias", &shadow_bias, 0.0f, 0.001f);
        }

        if (changed)
        {
            Renderer::instance->update_lights();
        }

        ImGui::TreePop();
    }
}

void Light3D::set_color(glm::vec3 color)
{
    this->color = color;
}

void Light3D::set_intensity(float value)
{
    this->intensity = value;
}

void Light3D::set_fading_enabled(bool value)
{
    this->fading_enabled = value;
}

void Light3D::set_cast_shadows(bool value)
{
    this->cast_shadows = value;
}
