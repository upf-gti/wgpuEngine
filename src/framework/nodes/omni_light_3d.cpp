#include "omni_light_3d.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "framework/nodes/mesh_instance_3d.h"
#include "framework/math/math_utils.h"
#include "framework/nodes/node_factory.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "imgui.h"

REGISTER_NODE_CLASS(OmniLight3D)

OmniLight3D::OmniLight3D() : Light3D()
{
    type = LIGHT_OMNI;
    node_type = "OmniLight3D";
    name = node_type + "_" + std::to_string(last_node_id++);

    debug_mesh_v = new MeshInstance3D();
    debug_mesh_v->set_frustum_culling_enabled(false);
    debug_mesh_v->set_scale(glm::vec3(range));

    debug_mesh_h = new MeshInstance3D();
    debug_mesh_h->set_frustum_culling_enabled(false);
    debug_mesh_h->set_scale(glm::vec3(range));
    debug_mesh_h->rotate(glm::rotate(transform.get_rotation(), static_cast<float>(PI / 2.0f), glm::vec3(1.0f, 0.0, 0.0)));

    Surface* debug_surface = new Surface();
    debug_surface->create_circle(0.5f, 32);

    debug_material = new Material();
    debug_material->set_color(glm::vec4(color, 1.0f));
    debug_material->set_type(MATERIAL_UNLIT);
    debug_material->set_topology_type(TOPOLOGY_LINE_STRIP);
    debug_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, debug_material));
    debug_surface->set_material(debug_material);

    debug_mesh_v->add_surface(debug_surface);
    debug_mesh_h->add_surface(debug_surface);
}

OmniLight3D::~OmniLight3D()
{
    delete debug_mesh_v;
    delete debug_mesh_h;
}

void OmniLight3D::render()
{
    Renderer::instance->add_renderable(debug_mesh_v, get_global_model() * debug_mesh_v->get_global_model());
    Renderer::instance->add_renderable(debug_mesh_h, get_global_model() * debug_mesh_h->get_global_model());

    Light3D::render();
}

void OmniLight3D::render_gui()
{
    bool changed = false;

    if (ImGui::TreeNodeEx("OmniLight3D"))
    {
        if (ImGui::SliderFloat("Range", &range, 0.f, 10.0f)) {
            debug_mesh_v->set_scale(glm::vec3(range));
            debug_mesh_h->set_scale(glm::vec3(range));
        }

        ImGui::TreePop();
    }

    Light3D::render_gui();
}

void OmniLight3D::set_color(glm::vec3 color)
{
    debug_material->set_color(glm::vec4(color, 1.0f));

    Light3D::set_color(color);
}

void OmniLight3D::set_range(float value)
{
    debug_mesh_v->set_scale(glm::vec3(value));
    debug_mesh_h->set_scale(glm::vec3(value));

    Light3D::set_range(value);
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
