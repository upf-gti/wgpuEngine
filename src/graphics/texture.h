#pragma once

#include <map>
#include <string>

#include "webgpu_context.h"

#include "hdre.h"

class Texture {

    WGPUTexture texture = nullptr;

    WGPUTextureDimension dimension = WGPUTextureDimension_Undefined;
    WGPUTextureFormat	 format = WGPUTextureFormat_Undefined;
    WGPUExtent3D		 size = {};
    WGPUTextureUsage	 usage = WGPUTextureUsage_None;
    uint32_t			 mipmaps = 1;

    WGPUAddressMode wrap_u = WGPUAddressMode_ClampToEdge;
    WGPUAddressMode wrap_v = WGPUAddressMode_ClampToEdge;

    std::string path;

public:

	~Texture();

	static WebGPUContext* webgpu_context;

    void load(const std::string& texture_path);

	void create(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, const void* data);

    static bool convert_to_rgba8unorm(uint32_t width, uint32_t height, WGPUTextureFormat src_format, void* src, uint8_t* dst);

    WGPUTexture     get_texture() { return texture; }
	WGPUTextureView get_view();
    uint32_t        get_mipmap_count() { return mipmaps; }

    void set_wrap_u(WGPUAddressMode wrap_u) { this->wrap_u = wrap_u; }
    void set_wrap_v(WGPUAddressMode wrap_v) { this->wrap_v = wrap_v; }

    WGPUAddressMode get_wrap_u() { return wrap_u; }
    WGPUAddressMode get_wrap_v() { return wrap_v; }

    void load_from_data(const std::string& name, int width, int height, void* data, WGPUTextureFormat p_format = WGPUTextureFormat_RGBA8Unorm);
    void load_from_hdre( HDRE* hdre );
};
