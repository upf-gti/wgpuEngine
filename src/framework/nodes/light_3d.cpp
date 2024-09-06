#include "light_3d.h"
#include "graphics/renderer.h"
#include "imgui.h"
#include "graphics/renderer.h"

Light3D::Light3D() : Node3D()
{
    animatable_properties["intensity"] = { AnimatablePropertyType::FLOAT32, &intensity };
    animatable_properties["color"] = { AnimatablePropertyType::FVEC4, &color };
}

Light3D::~Light3D()
{
    
}

void Light3D::render()
{
    if (intensity < 0.001f)
    {
        return;
    }
    if (type != LIGHT_DIRECTIONAL && range == 0.0f)
    {
        return;
    }

    Renderer::instance->add_light(this);

    Node3D::render();
}

void Light3D::render_gui()
{
    if (ImGui::TreeNodeEx("Light3D", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool changed = false;

        ImGui::ColorEdit3("Color", &color[0]);
        ImGui::SliderFloat("Intensity", &intensity, 0.f, 100.0f);
        ImGui::Checkbox("Cast Shadows", &cast_shadows);

        if (cast_shadows) {
            ImGui::SliderFloat("Shadow Bias", &shadow_bias, 0.0f, 0.001f);
        }

        ImGui::TreePop();
    }

    Node3D::render_gui();
}

void Light3D::set_color(glm::vec3 color)
{
    this->color = color;
}

void Light3D::set_intensity(float value)
{
    this->intensity = value;
}

void Light3D::set_range(float value)
{
    this->range = value;
}

void Light3D::set_fading_enabled(bool value)
{
    this->fading_enabled = value;
}

void Light3D::set_cast_shadows(bool value)
{
    this->cast_shadows = value;
}
