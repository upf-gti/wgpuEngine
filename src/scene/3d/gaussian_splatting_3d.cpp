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
