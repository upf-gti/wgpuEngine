#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <bit>

#include "spdlog/spdlog.h"

WebGPUContext* Texture::webgpu_context = nullptr;

Texture::~Texture() {
    if (texture) {
        wgpuTextureDestroy(texture);
    }
}

void Texture::create(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, uint8_t sample_count, const void* data)
{
    this->dimension = dimension;
    this->format = format;
    this->size = size;
    this->usage = usage;
    this->mipmaps = mipmaps;
    this->sample_count = sample_count;

    if (texture) {
        wgpuTextureDestroy(texture);
    }

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps, sample_count);

    if (data != nullptr) {
        webgpu_context->create_texture_mipmaps(texture, size, mipmaps, data);
    }
}

bool Texture::convert_to_rgba8unorm(uint32_t width, uint32_t height, WGPUTextureFormat src_format, void* src, uint8_t* dst)
{
    if (src_format == WGPUTextureFormat_RGBA8Unorm) {
        return false;
    }

    for (uint32_t i = 0; i < height; ++i) {
        for (uint32_t j = 0; j < width; ++j) {

            switch (src_format) {
            case WGPUTextureFormat_RGBA16Uint:
            {
                uint16_t* src_converted = reinterpret_cast<uint16_t*>(src);
                uint32_t pixel_pos = (j * 4) + (i * 4 * width);
                dst[pixel_pos + 0] = static_cast<uint8_t>((src_converted[pixel_pos + 0] / 65535.0f) * 255.0f);
                dst[pixel_pos + 1] = static_cast<uint8_t>((src_converted[pixel_pos + 1] / 65535.0f) * 255.0f);
                dst[pixel_pos + 2] = static_cast<uint8_t>((src_converted[pixel_pos + 2] / 65535.0f) * 255.0f);
                dst[pixel_pos + 3] = static_cast<uint8_t>((src_converted[pixel_pos + 3] / 65535.0f) * 255.0f);
                break;
            }
            default:
                assert(false);
                return false;
            }

        }
    }

    return true;
}

void Texture::load(const std::string& texture_path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &channels, 4);

    if (!data)
        return;

    path = texture_path;

    load_from_data(path, width, height, 1, data);

    stbi_image_free(data);

    spdlog::info("Texture loaded: {}", texture_path);
}

void Texture::load_hdr(const std::string& texture_path)
{
    int width, height, channels;
    float* data = stbi_loadf(texture_path.c_str(), &width, &height, &channels, 4);

    if (!data)
        return;

    path = texture_path;

    load_from_data(path, width, height, 1, data, false, WGPUTextureFormat_RGBA32Float);

    stbi_image_free(data);

    spdlog::info("Texture HDR loaded: {}", texture_path);
}

void Texture::load_from_data(const std::string& name, int width, int height, int array_layers, void* data, bool create_mipmaps, WGPUTextureFormat p_format)
{
    dimension = WGPUTextureDimension_2D;
    format = p_format;
    size = { (unsigned int)width, (unsigned int)height, (unsigned int)array_layers };
    usage = static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst);
    mipmaps = create_mipmaps ? std::bit_width(std::max(size.width, size.height)) : 1;

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps, 1);

    // Create mipmaps
    if (create_mipmaps) {
        WGPUTextureUsage mipmaps_usage = static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc);

        WGPUTexture texture_temp = webgpu_context->create_texture(dimension, WGPUTextureFormat_RGBA8Unorm, size, mipmaps_usage, mipmaps, 1);
        webgpu_context->create_texture_mipmaps(texture_temp, size, mipmaps, data, WGPUTextureViewDimension_2D, WGPUTextureFormat_RGBA8Unorm);

        for (uint32_t i = 0; i < mipmaps; ++i) {
            WGPUExtent3D mipmap_size;

            if (i > 0) {
                mipmap_size = {
                    size.width / (2 << (i - 1)),
                    size.height / (2 << (i - 1)),
                    size.depthOrArrayLayers
                };
            }
            else {
                mipmap_size = size;
            }

            webgpu_context->copy_texture_to_texture(texture_temp, texture, i, i, mipmap_size);
        }

        wgpuTextureRelease(texture_temp);
    }
    else {
        webgpu_context->upload_texture_mipmaps(texture, size, 0, data, {0, 0, 0});
    }
}

void Texture::load_from_hdre(HDRE* hdre)
{
    dimension = WGPUTextureDimension_2D;
    format = WGPUTextureFormat_RGBA32Float;
    size = { (unsigned int)hdre->width, (unsigned int)hdre->height, 6 };
    usage = static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst);
    mipmaps = 6; // num generated levels

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps, 1);

    for (uint32_t level = 0; level < 6; ++level)
    {
        for (uint32_t face = 0; face < 6; ++face)
        {
            sHDRELevel hdre_level = hdre->getLevel(level);
            void* data = hdre_level.faces[face];
            WGPUOrigin3D origin = { 0, 0, face };
            WGPUExtent3D tex_size = { (unsigned int)hdre_level.width, (unsigned int)hdre_level.height, 1 };
            webgpu_context->upload_texture_mipmaps(texture, tex_size, level, data, origin);
        }
    }
}

WGPUTextureView Texture::get_view(WGPUTextureViewDimension view_dimension, uint32_t base_mip_level, uint32_t mip_level_count, uint32_t base_array_layer, uint32_t array_layer_count)
{
    return webgpu_context->create_texture_view(texture, view_dimension, format, WGPUTextureAspect_All, base_mip_level, mip_level_count, base_array_layer, array_layer_count);
}
