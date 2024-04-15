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
    if (is_skinned && animated_uniform_data && invbind_uniform_data)
    {
        // This has to be optimized.. ~6 FPS!!
        const std::vector<glm::mat4x4>& animated_matrices = get_animated_data();
        const std::vector<glm::mat4x4>& inv_bind_matrices = get_invbind_data();
        auto webgpu_context = Renderer::instance->get_webgpu_context();
        webgpu_context->update_buffer(std::get<WGPUBuffer>(animated_uniform_data->data), 0, animated_matrices.data(), sizeof(glm::mat4x4) * animated_matrices.size());
        webgpu_context->update_buffer(std::get<WGPUBuffer>(invbind_uniform_data->data), 0, inv_bind_matrices.data(), sizeof(glm::mat4x4) * inv_bind_matrices.size());
    }

    Node3D::update(delta_time);
}

std::vector<glm::mat4x4> MeshInstance3D::get_animated_data()
{
    assert(skeleton && is_skinned);
    Pose& current_pose = skeleton->get_current_pose();
    return current_pose.get_global_matrices();
}

std::vector<glm::mat4x4> MeshInstance3D::get_invbind_data()
{
    assert(skeleton && is_skinned);
    return  skeleton->get_inv_bind_pose();
}

void MeshInstance3D::set_uniform_data(Uniform* animated_u, Uniform* invbind_u)
{
    animated_uniform_data = animated_u;
    invbind_uniform_data = invbind_u;
}
