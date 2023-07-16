#pragma once

#include <map>
#include <string>

#include "webgpu_context.h"

#include <webgpu/webgpu.h>

class Pipeline;

class Shader {

public:

	~Shader();

	static WebGPUContext* webgpu_context;

	void reload();

	static Shader* get(const std::string& shader_path);

	WGPUShaderModule get_module() const;

	void set_pipeline(Pipeline* pipeline);

private:
	static std::map<std::string, Shader*> shaders;

	void load(const std::string& shader_path);

	std::string path;

	WGPUShaderModule shader_module;

	// Pipeline that uses this shader
	Pipeline* pipeline_ref = nullptr;

	bool loaded = false;
};