#include "directional_light_3d.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "framework/nodes/mesh_instance_3d.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "framework/math/math_utils.h"

#include "imgui.h"

DirectionalLight3D::DirectionalLight3D() : Light3D()
{
    type = LIGHT_DIRECTIONAL;
    node_type = "DirectionalLight3D";
    name = node_type + "_" + std::to_string(last_node_id++);

    debug_mesh_v = new MeshInstance3D();
    debug_mesh_v->set_frustum_culling_enabled(false);

    debug_mesh_h = new MeshInstance3D();
    debug_mesh_h->set_frustum_culling_enabled(false);
    debug_mesh_h->rotate(glm::rotate(transform.get_rotation(), static_cast<float>(PI / 2.0f), glm::vec3(1.0f, 0.0, 0.0)));

    Surface* debug_surface = new Surface();
    debug_surface->create_arrow();

    debug_material = new Material();
    debug_material->set_color(glm::vec4(color, 1.0f));
    debug_material->set_type(MATERIAL_UNLIT);
    debug_material->set_topology_type(TOPOLOGY_LINE_STRIP);
    debug_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, debug_material));
    debug_surface->set_material(debug_material);

    debug_mesh_v->add_surface(debug_surface);
    debug_mesh_h->add_surface(debug_surface);
}

DirectionalLight3D::~DirectionalLight3D()
{
    delete debug_mesh_v;
    delete debug_mesh_h;
}

void DirectionalLight3D::render()
{
    // Use light transform to simplify rotation logic
    Renderer::instance->add_renderable(debug_mesh_v, get_global_model() * debug_mesh_v->get_global_model());
    Renderer::instance->add_renderable(debug_mesh_v, get_global_model() * debug_mesh_h->get_global_model());

    Light3D::render();
}

void DirectionalLight3D::render_gui()
{
    bool changed = false;

    if (ImGui::TreeNodeEx("DirectionalLight3D"))
    {
        if (ImGui::SliderFloat("Range", &range, 0.f, 10.0f)) {
        }

        ImGui::TreePop();
    }

    Light3D::render_gui();
}

void DirectionalLight3D::set_color(glm::vec3 color)
{
    debug_material->set_color(glm::vec4(color, 1.0f));

    Light3D::set_color(color);
}

sLightUniformData DirectionalLight3D::get_uniform_data()
{
    return {
        .position = get_translation(),
        .type = type,
        .color = color,
        .intensity = intensity,
        .direction = -get_global_model()[2]
    };
}
