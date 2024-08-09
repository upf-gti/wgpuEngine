#pragma once

#include <map>
#include <string>

#include "framework/resources/resource.h"

#include "webgpu_context.h"

#include "hdre.h"

class Texture : public Resource {

    WGPUTexture texture = nullptr;

    WGPUTextureDimension dimension = WGPUTextureDimension_Undefined;
    WGPUTextureFormat	 format = WGPUTextureFormat_Undefined;
    WGPUExtent3D		 size = {};
    WGPUTextureUsage	 usage = WGPUTextureUsage_None;
    uint32_t			 mipmaps = 1;
    uint8_t              sample_count = 1;

    WGPUAddressMode wrap_u = WGPUAddressMode_ClampToEdge;
    WGPUAddressMode wrap_v = WGPUAddressMode_ClampToEdge;

    std::string path;

public:

	~Texture();

	static WebGPUContext* webgpu_context;

    void load(const std::string& texture_path, bool is_srgb);
    void load_hdr(const std::string& texture_path);

	void create(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, uint8_t sample_count, const void* data);

    void generate_mipmaps(const void* data);

    static bool convert_to_rgba8unorm(uint32_t width, uint32_t height, WGPUTextureFormat src_format, void* src, uint8_t* dst);

    WGPUTexture     get_texture() { return texture; }
	WGPUTextureView get_view(WGPUTextureViewDimension view_dimension = WGPUTextureViewDimension_2D,
        uint32_t base_mip_level = 0, uint32_t mip_level_count = 1,
        uint32_t base_array_layer = 0, uint32_t array_layer_count = 1) const;


    uint32_t        get_mipmap_count() const { return mipmaps; }
    WGPUTextureDimension get_dimension() const { return dimension; }

    void set_wrap_u(WGPUAddressMode wrap_u) { this->wrap_u = wrap_u; }
    void set_wrap_v(WGPUAddressMode wrap_v) { this->wrap_v = wrap_v; }

    WGPUAddressMode get_wrap_u() const { return wrap_u; }
    WGPUAddressMode get_wrap_v() const { return wrap_v; }

    uint32_t get_width() const { return size.width; }
    uint32_t get_height() const { return size.height; }
    uint32_t get_array_layers() const { return size.depthOrArrayLayers; }

    WGPUTextureFormat get_format() const { return format; }

    WGPUExtent3D get_size() const { return size; }

    void load_from_data(const std::string& name, WGPUTextureDimension dimension, int width, int height, int array_layers, void* data, bool create_mipmaps = true, WGPUTextureFormat p_format = WGPUTextureFormat_RGBA8Unorm);
    void load_from_hdre( HDRE* hdre );
};
