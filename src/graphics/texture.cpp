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

bool Texture::convert_to_rgba8unorm(uint32_t width, uint32_t height, WGPUTextureFormat src_format, void* src, uint8_t* dst)
{
    if (src_format == WGPUTextureFormat_RGBA8Unorm) {
        return false;
    }

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {

            switch (src_format) {
            case WGPUTextureFormat_RGBA16Uint:
            {
                uint16_t* src_converted = reinterpret_cast<uint16_t*>(src);
                size_t pixel_pos = (j * 4) + (i * 4 * width);
                dst[pixel_pos + 0] = (src_converted[pixel_pos + 0] / 65535.0f) * 255.0f;
                dst[pixel_pos + 1] = (src_converted[pixel_pos + 1] / 65535.0f) * 255.0f;
                dst[pixel_pos + 2] = (src_converted[pixel_pos + 2] / 65535.0f) * 255.0f;
                dst[pixel_pos + 3] = (src_converted[pixel_pos + 3] / 65535.0f) * 255.0f;
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

    load_from_data(path, width, height, data);

    stbi_image_free(data);

    spdlog::info("Texture loaded: {}", texture_path);
}

void Texture::load_from_data(const std::string& name, int width, int height, void* data, WGPUTextureFormat p_format)
{
    dimension = WGPUTextureDimension_2D;
    format = p_format;
    size = { (unsigned int)width, (unsigned int)height, 1 };
    usage = static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding | WGPUTextureUsage_CopyDst);
    mipmaps = std::bit_width(std::max(size.width, size.height));

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps);

    webgpu_context->create_texture_mipmaps(texture, size, mipmaps, data, WGPUTextureViewDimension_2D, format);
}

void Texture::load_from_hdre(HDRE* hdre)
{
    dimension = WGPUTextureDimension_2D;
    format = WGPUTextureFormat_RGBA32Float;
    size = { (unsigned int)hdre->width, (unsigned int)hdre->height, 6 };
    usage = static_cast<WGPUTextureUsage>(WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst);
    mipmaps = 6; // num generated levels

    texture = webgpu_context->create_texture(dimension, format, size, usage, mipmaps);

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
