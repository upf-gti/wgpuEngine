#pragma once

#include "includes.h"

#include <webgpu/webgpu.h>

#include <variant>
#include <functional>

struct Uniform {

    Uniform();

    void destroy();

    std::variant<WGPUBuffer, WGPUSampler, WGPUTextureView> data;

    uint32_t binding = 0;
    uint32_t visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

    WGPUBufferBindingType buffer_binding_type = WGPUBufferBindingType_Uniform;
    uint64_t buffer_size = 0;

    WGPUTextureBindingLayout texture_binding_layout;
    WGPUStorageTextureBindingLayout storage_texture_binding_layout;

    bool is_storage_texture = false;

    WGPUBindGroupLayoutEntry get_bind_group_layout_entry() const;
    WGPUBindGroupEntry       get_bind_group_entry() const;
};