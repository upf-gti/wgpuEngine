#pragma once

#include <map>
#include <string>

#include "webgpu_context.h"

#include <webgpu/webgpu.h>

class Shader {

public:

	~Shader();

	static WebGPUContext* webgpu_context;

	void load(const std::string& shader_path);

	static Shader* get(const std::string& shader_path);

	WGPUShaderModule& get_module();

private:
	static std::map<std::string, Shader*> shaders;

	std::string path;

	WGPUShaderModule shader_module;

	bool loaded = false;
};