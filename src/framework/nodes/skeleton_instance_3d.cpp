#include "skeleton_instance_3d.h"

#include "graphics/renderer.h"


void SkeletonInstance3D::render()
{
    if (skeleton_helper) {

        Renderer::instance->add_renderable(skeleton_helper, get_global_model());

        Node3D::render();
    }

}

void SkeletonInstance3D::update(float delta_time)
{
    Node3D::update(delta_time);
}

void SkeletonInstance3D::set_skeleton(Skeleton* s)
{
    skeleton = s;
    init_skeleton_helper();
    
}

void SkeletonInstance3D::init_skeleton_helper() {
    unsigned int requiredVerts = 0;

    unsigned int numJoints = skeleton->get_bind_pose().size();
    std::vector<InterleavedData> vertices;
    Pose pose = skeleton->get_bind_pose();
    for (unsigned int i = 0; i < numJoints; ++i) {
        if (pose.get_parent(i) < 0) {
            continue;
        }

        requiredVerts += 2;
    }

    vertices.resize(0);
    glm::mat4x4 global_model = get_global_model();
    
    for (unsigned int i = 0; i < numJoints - 1; ++i) {
        InterleavedData data;
        if (pose.get_parent(i) < 0) {
            continue;
        }
        else if (pose.get_parent(pose.get_parent(i)) < 0)
            continue;
        
        data.position = pose.get_global_transform(i).position;// combine(mat4ToTransform(model), pose.get_global_transform(i)).position; 
        vertices.push_back(data);
        data.position = pose.get_global_transform(pose.get_parent(i)).position; //combine(mat4ToTransform(model), pose.get_global_transform(pose.get_parent(i))).position;
        vertices.push_back(data);
    }

    Surface* s = new Surface();
    s->create_from_vertices(vertices);
    skeleton_helper = new MeshInstance3D();
    skeleton_helper->add_surface(s);
    Material skeleton_material;
    skeleton_material.depth_read = false;
    skeleton_material.transparency_type = eTransparencyType::ALPHA_BLEND;
    skeleton_material.topology_type = eTopologyType::TOPOLOGY_LINE_LIST;
    skeleton_material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl", skeleton_material);
    skeleton_helper->set_surface_material_override(s, skeleton_material);
}
Skeleton* SkeletonInstance3D::get_skeleton() {
    return skeleton;
}
