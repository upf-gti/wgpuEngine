#include "joint_3d.h"

#include "graphics/renderer.h"
#include "graphics/renderer_storage.h"

#include "framework/nodes/mesh_instance_3d.h"
#include "framework/nodes/skeleton_instance_3d.h"
#include "framework/parsers/parse_obj.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include <glm/gtc/type_ptr.hpp>

Joint3D* Joint3D::selected_joint = nullptr;

Joint3D::Joint3D()
{
    node_type = "Joint3D";

    Material* material = new Material();
    material->set_depth_read(false);
    material->set_priority(0);
    material->set_transparency_type(ALPHA_BLEND);
    material->set_color(glm::vec4(1.0f));
    material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path));

    Material* selected_material = new Material();
    selected_material->set_depth_read(false);
    selected_material->set_priority(0);
    selected_material->set_transparency_type(ALPHA_BLEND);
    selected_material->set_color(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    selected_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path));

    mesh_instance = new MeshInstance3D();
    mesh_instance->set_frustum_culling_enabled(false);
    mesh_instance->add_surface(RendererStorage::get_surface("box"));
    mesh_instance->set_surface_material_override(mesh_instance->get_surface(0), material);

    selected_mesh_instance = new MeshInstance3D();
    selected_mesh_instance->set_frustum_culling_enabled(false);
    selected_mesh_instance->add_surface(RendererStorage::get_surface("box"));
    selected_mesh_instance->set_surface_material_override(selected_mesh_instance->get_surface(0), selected_material);
}

Joint3D::~Joint3D()
{
    delete mesh_instance;
    delete selected_mesh_instance;
}

Transform Joint3D::get_global_transform()
{
    assert(pose && parent);
    return Transform::combine(get_parent<Node3D*>()->get_global_transform(), pose->get_global_transform(index));
}

/*
*   Input is global transform -> has transform of skeleton instance + every joint from above
*   It has to set local transform for the joint
*/
void Joint3D::set_global_transform(const Transform& new_transform)
{
    // To joint space
    Transform local_transform = Transform::combine(Transform::inverse(get_parent<Node3D*>()->get_global_transform()), new_transform);

    // To local joint space
    int32_t parent_id = pose->get_parent(index);
    if (parent_id >= 0) {
        local_transform = Transform::combine(Transform::inverse(pose->get_global_transform(parent_id)), local_transform);
    }

    set_transform(local_transform);
}

void Joint3D::render()
{
    assert(pose && parent);
    Transform joint_global_transform = Transform::combine(get_parent<Node3D*>()->get_global_transform(), pose->get_global_transform(index));
    joint_global_transform.set_scale(glm::vec3(0.01f));
    bool selected = (Joint3D::selected_joint == this);
    Renderer::instance->add_renderable(selected ? selected_mesh_instance : mesh_instance, joint_global_transform.get_model());
}
