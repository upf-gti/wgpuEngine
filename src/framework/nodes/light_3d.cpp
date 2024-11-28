#include "light_3d.h"

#include "graphics/renderer.h"

#include "imgui.h"

#include <fstream>

Light3D::Light3D() : Node3D()
{
    animatable_properties["intensity"] = { AnimatablePropertyType::FLOAT32, &intensity };
    animatable_properties["range"] = { AnimatablePropertyType::FLOAT32, &range };
    animatable_properties["color"] = { AnimatablePropertyType::FVEC4, &color };

    collider_shape = COLLIDER_SHAPE_SPHERE;
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

void Light3D::clone(Node* new_node, bool copy)
{
    Node3D::clone(new_node, copy);

    Light3D* new_light = static_cast<Light3D*>(new_node);

    if (!copy) {
        // TODO
    }
    else {
        new_light->set_intensity(intensity);
        new_light->set_color(color);
        new_light->set_range(range);
        // Fading
        // ...
        // Shadows
        // ...
    }
}

void Light3D::render_gui()
{
    if (ImGui::TreeNodeEx("Light3D", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool changed = false;

        glm::vec3 gui_color = color;
        if (ImGui::ColorEdit3("Color", &gui_color[0])) {
            set_color(gui_color);
        }

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

void Light3D::serialize(std::ofstream& binary_scene_file)
{
    Node3D::serialize(binary_scene_file);

    /* TO serialize
    // Fading
    bool fading_enabled = false;
    float fade_begin = 10.0f;
    float fade_length = 1.0f;

    // Shadows
    bool cast_shadows = false;
    float shadow_bias = 0.001f;
    */

    binary_scene_file.write(reinterpret_cast<char*>(&intensity), sizeof(float));
    binary_scene_file.write(reinterpret_cast<char*>(&color), sizeof(glm::vec3));
    binary_scene_file.write(reinterpret_cast<char*>(&range), sizeof(float));
}

void Light3D::parse(std::ifstream& binary_scene_file)
{
    Node3D::parse(binary_scene_file);

    binary_scene_file.read(reinterpret_cast<char*>(&intensity), sizeof(float));
    binary_scene_file.read(reinterpret_cast<char*>(&color), sizeof(glm::vec3));
    binary_scene_file.read(reinterpret_cast<char*>(&range), sizeof(float));
}
