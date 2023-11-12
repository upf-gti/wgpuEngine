#pragma once

#include <map>
#include <string>

#include "webgpu_context.h"

class Texture {

public:

	~Texture();

	static WebGPUContext* webgpu_context;

    void load(const std::string& texture_path);

	void create(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, const void* data);

    WGPUTexture     get_texture() { return texture; }
	WGPUTextureView get_view();
    uint32_t        get_mipmap_count() { return mipmaps; }

    void load_from_data(const std::string& name, int width, int height, void* data);

private:

	WGPUTexture texture = nullptr;

	WGPUTextureDimension dimension;
	WGPUTextureFormat	 format;
	WGPUExtent3D		 size;
	WGPUTextureUsage	 usage;
	uint32_t			 mipmaps = 1;

	std::string path;
};
