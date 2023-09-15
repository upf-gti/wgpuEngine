#pragma once

#include <map>
#include <string>
#include <vector>

#include "webgpu_context.h"

class Pipeline;

class Shader {

public:

	~Shader();

	static WebGPUContext* webgpu_context;

	void reload();

	static Shader* get(const std::string& shader_path);
	static std::vector<std::string> get_for_reload(const std::string& shader_path);

	WGPUShaderModule get_module() const;

	void set_pipeline(Pipeline* pipeline);
	Pipeline* get_pipeline() { return pipeline_ref; }

	std::map<int, WGPUBindGroupLayout>&  get_bind_group_layouts()    { return bind_group_layouts; }
	std::vector<WGPUVertexBufferLayout>& get_vertex_buffer_layouts() { return vertex_buffer_layouts; }

	std::string get_path() { return path; }

	bool is_loaded() { return loaded; }

private:
	static std::map<std::string, Shader*> shaders;
	static std::map<std::string, std::vector<std::string>> library_references;

	bool load(const std::string& shader_path);
	void get_reflection_data(const std::string& shader_path, const std::string& shader_content);

	std::string path;

	WGPUShaderModule shader_module;

	std::map<int, WGPUBindGroupLayout> bind_group_layouts;

	std::vector<WGPUVertexAttribute>	vertex_attributes;
	std::vector<WGPUVertexBufferLayout> vertex_buffer_layouts;

	// Pipeline that uses this shader
	Pipeline* pipeline_ref = nullptr;

	bool loaded = false;
};