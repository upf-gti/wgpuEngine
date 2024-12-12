#include "skeleton_helper_3d.h"

#include "graphics/renderer_storage.h"

#include "framework/animation/skeleton.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include <glm/gtc/type_ptr.hpp>

SkeletonHelper3D::SkeletonHelper3D(Skeleton* new_skeleton, Node3D* parent) : MeshInstance3D()
{
    node_type = "SkeletonHelper3D";

    skeleton = new_skeleton;

    {
        set_frustum_culling_enabled(false);

        Surface* s = new Surface();
        s->set_name("Skeleton Helper");
        add_surface(s);

        inner_update();

        Material* skeleton_material = new Material();
        skeleton_material->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });
        skeleton_material->set_depth_read(false);
        skeleton_material->set_priority(0);
        skeleton_material->set_topology_type(eTopologyType::TOPOLOGY_LINE_LIST);
        skeleton_material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, skeleton_material));
        set_surface_material_override(s, skeleton_material);

        if (parent) {
            set_parent(parent);
        }
    }
}

void SkeletonHelper3D::inner_update()
{
    Surface* s = get_surface(0);

    sSurfaceData vertices;

    Pose& pose = skeleton->get_current_pose();
    size_t joint_count = pose.size();

    vertices.vertices.reserve(joint_count);

    for (size_t i = 0; i < joint_count; ++i) {
        if (pose.get_parent(i) >= 0) {
            vertices.vertices.push_back(pose.get_global_transform(pose.get_parent(i)).get_position());
        }
        else {
            vertices.vertices.push_back(pose.get_global_transform(i).get_position());
        }
    }

    s->update_surface_data(vertices);
}

void SkeletonHelper3D::update(float dt)
{
    inner_update();
}
