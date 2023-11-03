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
    spdlog::info("Loading shader: {}", texture_path);

    int width, height, channels;
    unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &channels, 4);

    if (!data)
        return;

    path = texture_path;

    load_from_data(path, width, height, data);

    stbi_image_free(data);

    //std::cout << " [OK]" << std::endl;
}

void Texture::load_from_data(const std::string& name, int width, int height, void* data)
{
    dimension = WGPUTextureDimension_2D;
    format = WGPUTextureFormat_RGBA8Unorm;
    size = { (unsigned int)width, (unsigned int)height, 1 };
    usage = static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst);
    mipmaps = 1;// std::bit_width(std::max(size.width, size.height));

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps);

    webgpu_context->create_texture_mipmaps(texture, size, mipmaps, data);
}

WGPUTextureView Texture::get_view()
{
    WGPUTextureViewDimension view_dimension;

    switch (dimension) {
    case WGPUTextureDimension_1D:
        view_dimension = WGPUTextureViewDimension_1D;
        break;
    case WGPUTextureDimension_2D:
        view_dimension = WGPUTextureViewDimension_2D;
        break;
    case WGPUTextureDimension_3D:
        view_dimension = WGPUTextureViewDimension_3D;
        break;
    default:
        spdlog::error("Texture View dimension not implemented");
        assert(0);
        break;
    }

    return webgpu_context->create_texture_view(texture, view_dimension, format);
}
