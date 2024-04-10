#include "skeleton_instance_3d.h"

#include "graphics/renderer.h"


void SkeletonInstance3D::set_skeleton(Skeleton* s)
{
    skeleton = s;
    init_skeleton_helper();
    
}

void SkeletonInstance3D::update(float dt) {
    init_skeleton_helper();
}

void SkeletonInstance3D::init_skeleton_helper() {
    
    Surface* s = new Surface();
    std::vector<InterleavedData>& vertices = s->get_vertices();

    if (get_surfaces().size()) {
        s = get_surface(0);
    }
    else {
        add_surface(s);
    }

    unsigned int numJoints = skeleton->get_current_pose().size();
    Pose pose = skeleton->get_current_pose();
    vertices.resize(0);
    glm::mat4x4 global_model = get_global_model();
    
    for (unsigned int i = 0; i < numJoints ; ++i) {
        InterleavedData data;
        if (pose.get_parent(i) < 0) {
            continue;
        }
 
        data.position = pose.get_global_transform(i).position;
        vertices.push_back(data);
        data.position = pose.get_global_transform(pose.get_parent(i)).position;
        vertices.push_back(data);
    }

     s->create_from_vertices(vertices);
    
 
    Material skeleton_material;
    skeleton_material.depth_read = false;
    skeleton_material.transparency_type = eTransparencyType::ALPHA_BLEND;
    skeleton_material.topology_type = eTopologyType::TOPOLOGY_LINE_LIST;
    skeleton_material.shader = RendererStorage::get_shader("data/shaders/mesh_color.wgsl", skeleton_material);
    set_surface_material_override(s, skeleton_material);
}
Skeleton* SkeletonInstance3D::get_skeleton() {
    return skeleton;
}
