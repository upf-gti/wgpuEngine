#include "mesh_instance_3d.h"

#include "graphics/renderer.h"
#include "framework/animation/skeleton.h"

MeshInstance3D::MeshInstance3D() : Node3D()
{

}

MeshInstance3D::~MeshInstance3D()
{
    
}

void MeshInstance3D::render()
{
    Renderer::instance->add_renderable(this, get_global_model());

    Node3D::render();
}

void MeshInstance3D::update(float delta_time)
{
    // Update GPU data
    if (is_skinned && bones_uniform_data)
    {
        // This has to be optimized.. ~6 FPS!!
        /*const std::vector<glm::mat4x4>& animated_matrices = get_bone_data();
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        webgpu_context->update_buffer(std::get<WGPUBuffer>(bones_uniform_data->data), 0, animated_matrices.data(), sizeof(glm::mat4x4) * animated_matrices.size());*/
    }

    Node3D::update(delta_time);
}

std::vector<glm::mat4x4> MeshInstance3D::get_bone_data()
{
    Pose& current_pose = skeleton->get_current_pose();
    Pose bind_pose = skeleton->get_bind_pose();

    Transform t = current_pose.get_local_transform(2);
    t.rotation = glm::quat(0.0f, 0.707f, 0.0f, 0.707f);
    current_pose.set_local_transform(2, t);

    const std::vector<InterleavedData>& vertices = get_surface(0)->get_vertices();

    std::vector<glm::mat4x4> animated_matrices;
    animated_matrices.resize(std::max((int)current_pose.size(), 4), glm::mat4x4(1.0f));

    for (size_t i = 0; i < vertices.size(); i++) {

        glm::ivec4 joints = vertices[i].joints;

        Transform animated_transform = combine(current_pose[joints.x], inverse(bind_pose[joints.x]));
        animated_matrices[joints.x] = transformToMat4(animated_transform);

        animated_transform = combine(current_pose[joints.y], inverse(bind_pose[joints.y]));
        animated_matrices[joints.y] = transformToMat4(animated_transform);
        if (current_pose.size() > 2 && joints.z < animated_matrices.size()) {
            animated_transform = combine(current_pose[joints.z], inverse(bind_pose[joints.z]));
            animated_matrices[joints.z] = transformToMat4(animated_transform);
        }
        if (current_pose.size() > 3 && joints.w < animated_matrices.size()) {
            animated_transform = combine(current_pose[joints.w], inverse(bind_pose[joints.w]));

            animated_matrices[joints.w] = transformToMat4(animated_transform);
        }
    }

    return animated_matrices;
}

void MeshInstance3D::set_uniform_data(Uniform* u)
{
    bones_uniform_data = u;
}
