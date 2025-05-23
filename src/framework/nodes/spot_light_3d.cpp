#include "spot_light_3d.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "framework/math/math_utils.h"
#include "framework/nodes/mesh_instance_3d.h"
#include "framework/nodes/node_factory.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "imgui.h"

#include <fstream>

REGISTER_NODE_CLASS(SpotLight3D)

SpotLight3D::SpotLight3D() : Light3D()
{
    type = LIGHT_SPOT;
    node_type = "SpotLight3D";
    name = node_type + "_" + std::to_string(last_node_id++);

    animatable_properties["inner_cone_angle"] = { AnimatablePropertyType::FLOAT32, &inner_cone_angle };
    animatable_properties["outer_cone_angle"] = { AnimatablePropertyType::FLOAT32, &outer_cone_angle };
}

SpotLight3D::~SpotLight3D()
{
    
}

void SpotLight3D::create_debug_render_cone()
{
    float radius = range * sin(outer_cone_angle);
    float height = sqrt(range * range - radius * radius);
    debug_surface->create_cone(radius, height, 32);
}

void SpotLight3D::render()
{
#ifndef NDEBUG
    if (debug_material) {
        Renderer::instance->add_renderable(debug_mesh->get_mesh_instance(), get_global_model() * debug_mesh->get_global_model());
    }
#endif
}

void SpotLight3D::clone(Node* new_node, bool copy)
{
    Light3D::clone(new_node, copy);

    SpotLight3D* new_light = static_cast<SpotLight3D*>(new_node);

    if (!copy) {
        // TODO
    }
    else {
        new_light->set_inner_cone_angle(inner_cone_angle);
        new_light->set_outer_cone_angle(outer_cone_angle);
    }
}

void SpotLight3D::render_gui()
{
    bool changed = false;
    float pi_2 = glm::pi<float>() * 0.5f;

    if (ImGui::TreeNodeEx("SpotLight3D"))
    {
        if (ImGui::SliderFloat("Range", &range, 0.f, 10.0f)) {
            if (debug_material) {
                create_debug_render_cone();
            }
        }

        ImGui::SliderFloat("Inner Angle", &inner_cone_angle, 0.f, pi_2);

        if (ImGui::SliderFloat("Outer Angle", &outer_cone_angle, 0.f, pi_2)) {
            if (debug_material) {
                create_debug_render_cone();
            }
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
    if (debug_material) {
        create_debug_render_cone();
    }

    Light3D::set_range(value);
}

void SpotLight3D::set_inner_cone_angle(float value)
{
    this->inner_cone_angle = value;
}

void SpotLight3D::set_outer_cone_angle(float value)
{
    this->outer_cone_angle = value;

    if (debug_material) {
        create_debug_render_cone();
    }
}

void SpotLight3D::serialize(std::ofstream& binary_scene_file)
{
    Light3D::serialize(binary_scene_file);

    binary_scene_file.write(reinterpret_cast<char*>(&inner_cone_angle), sizeof(float));
    binary_scene_file.write(reinterpret_cast<char*>(&outer_cone_angle), sizeof(float));
}

void SpotLight3D::parse(std::ifstream& binary_scene_file)
{
    Light3D::parse(binary_scene_file);

    binary_scene_file.read(reinterpret_cast<char*>(&inner_cone_angle), sizeof(float));
    binary_scene_file.read(reinterpret_cast<char*>(&outer_cone_angle), sizeof(float));

    if (debug_material) {
        debug_material->set_color(glm::vec4(color, 1.0f));
        create_debug_render_cone();
    }
}

void SpotLight3D::create_debug_meshes()
{
    debug_mesh = new MeshInstance3D();
    debug_mesh->set_frustum_culling_enabled(false);
    debug_mesh->set_scale(glm::vec3(range));

    debug_surface = new Surface();
    create_debug_render_cone();

    debug_material = new Material();
    debug_material->set_color(glm::vec4(color, 1.0f));
    debug_material->set_type(MATERIAL_UNLIT);
    debug_material->set_topology_type(TOPOLOGY_LINE_STRIP);
    debug_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries, debug_material));
    debug_surface->set_material(debug_material);

    debug_mesh->add_surface(debug_surface);
}
