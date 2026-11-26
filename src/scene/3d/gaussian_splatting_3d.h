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

    void initialize(uint32_t splat_count);

    virtual void render() override;
    virtual void update(float delta_time) override;

    void sort(WGPUComputePassEncoder compute_pass);

    WGPUBuffer get_render_buffer()
    {
        return render_buffer;
    }

    WGPUBuffer get_ids_buffer()
    {
        return ids_buffer;
    }

    WGPUBindGroup get_render_bindgroup()
    {
        return render_bindgroup;
    }

    bool is_covariance_calculated() { return covariance_calculated; }

    void set_render_buffers(const std::vector<glm::vec4>& positions, const std::vector<glm::vec4>& colors);
    void set_covariance_buffers(const std::vector<glm::quat>& rotations, const std::vector<glm::vec4>& scales);

    void calculate_basis(WGPUComputePassEncoder compute_pass);
    void calculate_covariance(WGPUComputePassEncoder compute_pass);

    uint32_t get_splat_count();
    uint32_t get_padded_splat_count();
    uint32_t get_splats_render_bytes_size();
    uint32_t get_ids_render_bytes_size();
};
