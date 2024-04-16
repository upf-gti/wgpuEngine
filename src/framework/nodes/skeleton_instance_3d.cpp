#include "skeleton_instance_3d.h"

#include "graphics/renderer.h"

SkeletonInstance3D::SkeletonInstance3D()
{

}

void SkeletonInstance3D::set_skeleton(Skeleton* s)
{
    skeleton = s;
    init_helper();
}

void SkeletonInstance3D::update(float dt)
{
    update_helper();

    Node3D::update(dt);
}

void SkeletonInstance3D::set_joint_nodes(const std::vector<Node3D*>& new_joint_nodes)
{
    joint_nodes = new_joint_nodes;
}

Node* SkeletonInstance3D::get_node(std::vector<std::string>& path_tokens)
{
    // path_tokens[0] -> skeleton instance
    // path_tokens[1] -> node_name
    // path_tokens[2] -> property

    // TODO: get joint node from list (special case for skeleton instances)

    /*for (Node* child : children) {

        if (child->get_name() == path_tokens[0]) {
            path_tokens.erase(path_tokens.begin());
            return child->get_node(path_tokens);
        }
    }*/

    return nullptr;
}

void SkeletonInstance3D::update_helper()
{
    // Update helper
    {
        Surface* s = get_surface(0);

        std::vector<InterleavedData>& vertices = s->get_vertices();
        vertices.resize(0);
        vertices.clear();

        unsigned int numJoints = skeleton->get_current_pose().size();
        Pose pose = skeleton->get_current_pose();

        glm::mat4x4 global_model = get_global_model();

        for (unsigned int i = 0; i < numJoints; ++i) {
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
    }
}

void SkeletonInstance3D::init_helper()
{
    Surface* s = new Surface();
    add_surface(s);

    std::vector<InterleavedData>& vertices = s->get_vertices();

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

Skeleton* SkeletonInstance3D::get_skeleton()
{
    return skeleton;
}
