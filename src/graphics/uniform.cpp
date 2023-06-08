#include "uniform.h"

Uniform::Uniform()
{
    texture_binding_layout.sampleType = WGPUTextureSampleType_Float;
    texture_binding_layout.viewDimension = WGPUTextureViewDimension_2D;
    texture_binding_layout.multisampled = false;

    storage_texture_binding_layout.access = WGPUStorageTextureAccess_WriteOnly;
    storage_texture_binding_layout.format = WGPUTextureFormat_RGBA8Unorm;
    storage_texture_binding_layout.viewDimension = WGPUTextureViewDimension_2D;
}

void Uniform::destroy()
{
    if (std::holds_alternative<WGPUBuffer>(data)) {
        wgpuBufferDestroy(std::get<WGPUBuffer>(data));
    }
    else if (std::holds_alternative<WGPUTextureView>(data)) {
        wgpuTextureViewRelease(std::get<WGPUTextureView>(data));
    }
}

WGPUBindGroupLayoutEntry Uniform::get_bind_group_layout_entry() const
{
    WGPUBindGroupLayoutEntry bindingLayout = {};

    // The binding index as used in the @binding attribute in the shader
    bindingLayout.binding = binding;
    // The stage that needs to access this resource
    bindingLayout.visibility = visibility;

    if (std::holds_alternative<WGPUBuffer>(data)) {
        bindingLayout.buffer.type = buffer_binding_type;
    }
    else
        if (std::holds_alternative<WGPUTextureView>(data)) {
            if (is_storage_texture) {
                bindingLayout.storageTexture = storage_texture_binding_layout;
            }
            else {
                bindingLayout.texture = texture_binding_layout;
            }
        }

    return bindingLayout;
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
    else
        if (std::holds_alternative<WGPUTextureView>(data)) {
            bindingGroup.textureView = std::get<WGPUTextureView>(data);
        }

    return bindingGroup;
}