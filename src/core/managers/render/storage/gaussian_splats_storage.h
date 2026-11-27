#pragma once

#include "core/managers/render/kernels/radix_sort_kernel.h"
#include "core/managers/render/render_lists_data.h"

#include "webgpu/webgpu.h"

#include "graphics/uniform.h"

class GaussianSplatsStorage {
    class RenderMethod;

public:
    Error initialize();

    void create_buffers(SplatData* splat_data);

    const Pipeline& get_render_pipeline() const;
    const Pipeline& get_basis_pipeline() const;
    const Pipeline& get_covariance_pipeline() const;

    struct SplatBuffers {
        ~SplatBuffers();

        uint32_t splat_count = 0;
        uint32_t padded_splat_count = 0;
        uint32_t workgroup_size = 0;

        WGPUBuffer render_buffer = nullptr;
        WGPUBuffer ids_buffer = nullptr;

        // Render
        Uniform model_uniform;
        Uniform centroid_uniform;
        Uniform basis_uniform;
        Uniform color_uniform;
        WGPUBindGroup render_bindgroup = nullptr;

        // Covariance
        Uniform rotations_uniform;
        Uniform scales_uniform;
        Uniform covariance_uniform;
        Uniform splat_count_uniform;
        WGPUBindGroup covariance_bindgroup = nullptr;

        // Basis
        WGPUBuffer distances_buffer = nullptr;

        Uniform basis_distances_uniform;
        Uniform ids_basis_uniform;
        WGPUBindGroup basis_uniform_bindgroup = nullptr;

        RadixSortKernel* radix_sort_kernel = nullptr;

        bool covariance_calculated = false;

        void initialize(SplatData* splat_data, Shader* covariance_shader, Shader* basis_shader, Shader* splats_render_shader);

        uint32_t get_splat_count()
        {
            return splat_count;
        }

        uint32_t get_splats_render_bytes_size()
        {
            return sizeof(float) * 8;
        }

        uint32_t get_ids_render_bytes_size()
        {
            return sizeof(uint32_t) * padded_splat_count;
        }
    };

private:
    Shader* covariance_shader = nullptr;
    Pipeline covariance_pipeline;

    Shader* basis_shader = nullptr;
    Pipeline basis_pipeline;

    Shader* splats_render_shader = nullptr;
    Pipeline splats_render_pipeline;
};
