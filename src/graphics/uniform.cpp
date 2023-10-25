#include "uniform.h"

Uniform::Uniform()
{
}

Uniform::~Uniform()
{
    destroy();
}

void Uniform::destroy()
{
    if (std::holds_alternative<WGPUBuffer>(data)) {
        wgpuBufferDestroy(std::get<WGPUBuffer>(data));
    }
    else if (std::holds_alternative<WGPUTextureView>(data)) {
        wgpuTextureViewRelease(std::get<WGPUTextureView>(data));
    }
    else if (std::holds_alternative<WGPUSampler>(data)) {
        wgpuSamplerRelease(std::get<WGPUSampler>(data));
    }

    data = {};
}

WGPUBindGroupEntry Uniform::get_bind_group_entry() const
{
    // Create a binding
    WGPUBindGroupEntry bindingGroup = {};

    // The index of the binding (the entries in bindGroupDesc can be in any order)
    bindingGroup.binding = binding;

    if (std::holds_alternative<WGPUBuffer>(data)) {
        // The buffer it is actually bound to
        bindingGroup.buffer = std::get<WGPUBuffer>(data);
        // We can specify an offset within the buffer, so that a single buffer can hold
        // multiple uniform blocks.
        bindingGroup.offset = 0;
        // And we specify again the size of the buffer.
        bindingGroup.size = buffer_size;
    }
    else if (std::holds_alternative<WGPUTextureView>(data)) {
        bindingGroup.textureView = std::get<WGPUTextureView>(data);
    }
    else if (std::holds_alternative<WGPUSampler>(data)) {
        bindingGroup.sampler = std::get<WGPUSampler>(data);
    }

    return bindingGroup;
}
