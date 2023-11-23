#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <bit>

WebGPUContext* Texture::webgpu_context = nullptr;

Texture::~Texture() {
    if (texture) {
        wgpuTextureDestroy(texture);
    }
}

void Texture::create(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, const void* data)
{
    this->dimension = dimension;
    this->format = format;
    this->size = size;
    this->usage = usage;
    this->mipmaps = mipmaps;

    if (texture) {
        wgpuTextureDestroy(texture);
    }

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps);

    if (data != nullptr) {
        webgpu_context->create_texture_mipmaps(texture, size, mipmaps, data);
    }
}

void Texture::load(const std::string& texture_path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &channels, 4);

    if (!data)
        return;

    path = texture_path;

    load_from_data(path, width, height, data);

    stbi_image_free(data);

    spdlog::info("Texture loaded: {}", texture_path);
}

void Texture::load_from_data(const std::string& name, int width, int height, void* data, WGPUTextureFormat p_format, int faces)
{
    dimension = WGPUTextureDimension_2D;
    format = p_format;
    size = { (unsigned int)width, (unsigned int)height, (unsigned int)faces };
    usage = static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding | WGPUTextureUsage_CopyDst);
    mipmaps = (p_format == WGPUTextureFormat_RGBA8Unorm) ? std::bit_width(std::max(size.width, size.height)) : 1;

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps);

    float** pixel_data = (float**)data;

    if (size.depthOrArrayLayers > 1) // If it's cubemap basically...
    {
        for( uint32_t i = 0; i < size.depthOrArrayLayers; ++i )
            webgpu_context->create_texture_mipmaps(texture, size, mipmaps, (const void*)pixel_data[i], WGPUTextureViewDimension_Cube, format, {0, 0, i});
    }
    else
    {
        webgpu_context->create_texture_mipmaps(texture, size, mipmaps, data);
    }
}

WGPUTextureView Texture::get_view()
{
    WGPUTextureViewDimension view_dimension;
    uint32_t array_layer_count = 1;

    switch (dimension) {
    case WGPUTextureDimension_1D:
        view_dimension = WGPUTextureViewDimension_1D;
        break;
    case WGPUTextureDimension_2D:
        view_dimension = size.depthOrArrayLayers > 1 ? WGPUTextureViewDimension_Cube : WGPUTextureViewDimension_2D;
        if (view_dimension == WGPUTextureViewDimension_Cube)
            array_layer_count = size.depthOrArrayLayers;
        break;
    case WGPUTextureDimension_3D:
        view_dimension = WGPUTextureViewDimension_3D;
        break;
    default:
        spdlog::error("Texture View dimension not implemented");
        assert(0);
        break;
    }

    return webgpu_context->create_texture_view(texture, view_dimension, format, WGPUTextureAspect_All, mipmaps, 0, array_layer_count);
}
