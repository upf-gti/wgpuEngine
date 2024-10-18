#pragma once

#include "includes.h"

#include "graphics/uniform.h"
#include "graphics/pipeline.h"
#include "graphics/kernels/radix_sort_kernel.h"

#include "framework/nodes/node_3d.h"

class Shader;

struct sGSRenderData {
    glm::vec2 position;
    uint32_t id;
};

class GSNode : public Node3D {

    uint32_t splat_count = 0;
    uint32_t padded_splat_count = 0;
    uint32_t workgroup_size = 0;

    WGPUBuffer render_buffer = nullptr;
    WGPUBuffer ids_buffer;

    // Render
    Uniform model_uniform;
    Uniform centroid_uniform;
    Uniform basis_uniform;
    Uniform color_uniform;
    WGPUBindGroup render_bindgroup;

    // Covariance
    Uniform rotations_uniform;
    Uniform scales_uniform;
    Uniform covariance_uniform;
    Uniform splat_count_uniform;
    WGPUBindGroup covariance_bindgroup;

    // Basis
    WGPUBuffer distances_buffer;

    Uniform basis_distances_uniform;
    Uniform ids_basis_uniform;
    WGPUBindGroup basis_uniform_bindgroup;

    RadixSortKernel* radix_sort_kernel = nullptr;

    Shader* covariance_shader = nullptr;
    Pipeline covariance_pipeline;

    Shader* basis_shader = nullptr;
    Pipeline basis_pipeline;

    bool covariance_calculated = false;

public:

    bool is_skinned = false;

    GSNode();
    ~GSNode();

    void initialize(uint32_t splat_count);

    virtual void render() override;
    virtual void update(float delta_time) override;

    void sort(WGPUComputePassEncoder compute_pass);

    WGPUBuffer get_render_buffer() {
        return render_buffer;
    }

    WGPUBuffer get_ids_buffer() {
        return ids_buffer;
    }

    WGPUBindGroup get_render_bindgroup() {
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
