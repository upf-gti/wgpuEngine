#include "gaussian_splatting_3d.h"

#include "core/managers/render/render_api.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/render/storage/render_storage.h"

#include "framework/math/math_utils.h"
#include "scene/main/node_factory.h"

REGISTER_NODE_CLASS(GaussianSplatting3D)

void GaussianSplatting3D::notification(int notification_id)
{
    switch (notification_id) {
        case NOTIFICATION_ENTERS_TREE:
            splat_data_ref.data.name = name;
            splat_data_ref.data.global_matrix = get_global_transform().get_model();
            RENDER_MANAGER->add_gaussian_splatting(&splat_data_ref);
            break;
        case NOTIFICATION_EXITS_TREE:
            RENDER_MANAGER->remove_gaussian_splatting(splat_data_ref.idx);
            break;
        case NOTIFICATION_TRANSFORM_DIRTY:
            splat_data_ref.data.global_matrix = get_global_transform().get_model();
            RENDER_MANAGER->set_gaussian_splattings_dirty();
            break;
    }

    Node::notification(notification_id);
}

GaussianSplatting3D::GaussianSplatting3D()
{
}

GaussianSplatting3D::~GaussianSplatting3D()
{
}

void GaussianSplatting3D::initialize(uint32_t splat_count)
{
}

void GaussianSplatting3D::render()
{
    RenderAPI::get_singleton()->buffer_update(std::get<WGPUBuffer>(model_uniform.data), 0, &get_global_model()[0], sizeof(glm::mat4x4));

    WGPUCommandEncoder command_encoder = RENDER_MANAGER->get_main_command_encoder();

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;

    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    if (!covariance_calculated) {
        calculate_covariance(compute_pass);
    }

    calculate_basis(compute_pass);
    sort(compute_pass);

    wgpuComputePassEncoderEnd(compute_pass);
    wgpuComputePassEncoderRelease(compute_pass);

    //RENDER_MANAGER->add_splat_scene(this);
}

void GaussianSplatting3D::update(float delta_time)
{
}

void GaussianSplatting3D::sort(WGPUComputePassEncoder compute_pass)
{
    radix_sort_kernel->dispatch(compute_pass);
}

void GaussianSplatting3D::set_render_buffers(const std::vector<glm::vec4>& positions, const std::vector<glm::vec4>& colors)
{
    splat_data_ref.data.positions = positions;
    splat_data_ref.data.colors = colors;

    RENDER_MANAGER->set_gaussian_splattings_dirty();
}

void GaussianSplatting3D::set_covariance_buffers(const std::vector<glm::quat>& rotations, const std::vector<glm::vec4>& scales)
{
    splat_data_ref.data.rotations = rotations;
    splat_data_ref.data.scales = scales;

    RENDER_MANAGER->set_gaussian_splattings_dirty();
}

void GaussianSplatting3D::calculate_covariance(WGPUComputePassEncoder compute_pass)
{
    if (!covariance_pipeline.set(compute_pass)) {
        return;
    }

    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, covariance_bindgroup, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroup_size, 1, 1);

    covariance_calculated = true;
}

void GaussianSplatting3D::calculate_basis(WGPUComputePassEncoder compute_pass)
{
    uint32_t camera_stride_offset = 0;

    if (!basis_pipeline.set(compute_pass)) {
        return;
    }

    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, basis_uniform_bindgroup, 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(compute_pass, 1, Renderer::instance->get_compute_camera_bind_group(), 1, &camera_stride_offset);
    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroup_size, 1, 1);
}
