#pragma once

#include <map>
#include <string>

#include "webgpu_context.h"

class Texture {

public:

	~Texture();

	static WebGPUContext* webgpu_context;

	void create(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, const void* data);

	static Texture* get(const std::string& texture_path);

    WGPUTexture     get_texture() { return texture; }
	WGPUTextureView get_view();

    void load_from_data(const std::string& name, int width, int height, void* data);

private:
	static std::map<std::string, Texture*> textures;

	void load(const std::string& texture_path);

	WGPUTexture texture = nullptr;

	WGPUTextureDimension dimension;
	WGPUTextureFormat	 format;
	WGPUExtent3D		 size;
	WGPUTextureUsage	 usage;
	uint32_t			 mipmaps = 1;

	std::string path;
};
