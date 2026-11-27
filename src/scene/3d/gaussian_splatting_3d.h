#pragma once

#include "includes.h"

#include "core/managers/render/kernels/radix_sort_kernel.h"
#include "core/managers/render/render_lists_data.h"

#include "graphics/pipeline.h"
#include "graphics/uniform.h"

#include "scene/3d/node_3d.h"

class Shader;

struct sGSRenderData {
    glm::vec2 position;
    uint32_t id;
};

class GaussianSplatting3D : public Node3D {
private:
    void notification(int notification_id) override;

    SplatDataRef splat_data_ref;

public:
    GaussianSplatting3D();
    ~GaussianSplatting3D();

    void set_render_buffers(const std::vector<glm::vec4>& positions, const std::vector<glm::vec4>& colors);
    void set_covariance_buffers(const std::vector<glm::quat>& rotations, const std::vector<glm::vec4>& scales);
};
