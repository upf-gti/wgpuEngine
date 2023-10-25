#pragma once

#include "includes.h"

#include <variant>
#include <functional>

struct Uniform {

    Uniform();
    ~Uniform();

    void destroy();

    std::variant<std::monostate, WGPUBuffer, WGPUSampler, WGPUTextureView> data;

    uint32_t binding = 0;
    uint64_t buffer_size = 0;

    WGPUBindGroupEntry       get_bind_group_entry() const;
};
