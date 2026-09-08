#include "light_3d.h"

#include "graphics/renderer.h"

#include "imgui.h"

#include <fstream>

Light3D::Light3D() : Node3D()
{
    animatable_properties["intensity"] = { AnimatablePropertyType::FLOAT32, &intensity };
    animatable_properties["range"] = { AnimatablePropertyType::FLOAT32, &range, std::bind(&Light3D::on_set_range, this) };
    animatable_properties["color"] = { AnimatablePropertyType::FVEC4, &color, std::bind(&Light3D::on_set_color, this) };

    collider_shape = COLLIDER_SHAPE_SPHERE;

    light_camera.set_orthographic(-10.0f, 10.0f, -10.0f, 10.0f, 20.0f, 0.1f);
}

Light3D::~Light3D()
{
    if (shadow_depth_texture) {
        wgpuTextureRelease(shadow_depth_texture);
        wgpuTextureViewRelease(shadow_depth_texture_view);
    }
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

void Light3D::get_uniform_data(sLightUniformData& data)
{
    const Transform& global_transform = get_global_transform();

    // By now all light cameras will be orthographic
    light_camera.look_at(get_global_transform().get_position(),
        global_transform.get_position() + global_transform.get_front(),
        glm::vec3(0.0f, 1.0f, 0.0f));

    data.position = global_transform.get_position();
    data.view_proj = light_camera.get_view_projection();
    data.type = type;
    data.color = color;
    data.intensity = intensity;
    data.range = range;
    data.shadow_bias = shadow_bias;
    data.cast_shadows = cast_shadows ? 1 : 0;
}

void Light3D::render_gui()
{
    if (ImGui::TreeNodeEx("Light3D", ImGuiTreeNodeFlags_DefaultOpen))
    {
        glm::vec3 gui_color = color;
        if (ImGui::ColorEdit3("Color", &gui_color[0])) {
            set_color(gui_color);
        }

        ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.f, 100.0f);
        if (ImGui::DragFloat("Range", &range, 0.1f, 0.f, 10.0f)) {
            on_set_range();
        }
        ImGui::Checkbox("Cast Shadows", &cast_shadows);

        if (cast_shadows) {

            ImGui::DragFloat("Shadow Bias", &shadow_bias, 0.0001f, 0.0f, 0.01f);

            if (ImGui::TreeNodeEx("Shadow Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool frustum_changed = false;

                glm::vec4 ortho_f = { light_camera.get_left(), light_camera.get_right(), light_camera.get_bottom(), light_camera.get_top() };
                frustum_changed |= ImGui::DragFloat4("Frustum plane", &ortho_f.x, 0.1f, -60.f, 60.0f);
                float near = light_camera.get_near();
                frustum_changed |= ImGui::DragFloat("Near", &near, 0.1f, -60.f, 60.0f);
                float far = light_camera.get_far();
                frustum_changed |= ImGui::DragFloat("Far", &far, 0.1f, -60.f, 60.0f);

                if (frustum_changed) {
                    light_camera.set_orthographic(ortho_f.x, ortho_f.y, ortho_f.z, ortho_f.w, near, far);
                }

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    Node3D::render_gui();
}

void Light3D::set_color(const glm::vec3& color)
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

void Light3D::on_set_color()
{
    set_color(color);
}

void Light3D::set_cast_shadows(bool new_cast_shadows)
{
    this->cast_shadows = new_cast_shadows;
}

void Light3D::set_shadow_bias(float new_shadow_bias)
{
    this->shadow_bias = new_shadow_bias;
}

void Light3D::create_shadow_data()
{
    if (shadow_depth_texture) {
        wgpuTextureRelease(shadow_depth_texture);
        wgpuTextureViewRelease(shadow_depth_texture_view);
    }

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    shadow_depth_texture = webgpu_context->create_texture(
        WGPUTextureDimension_2D,
        WGPUTextureFormat_Depth32Float,
        { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1 },
        WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc,
        1,
        1
    );

    shadow_depth_texture_view = webgpu_context->create_texture_view(
        shadow_depth_texture,
        WGPUTextureViewDimension_2D,
        WGPUTextureFormat_Depth32Float,
        WGPUTextureAspect_All,
        0,
        1,
        0,
        1,
        "shadow_map"
    );
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
