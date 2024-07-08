#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <variant>
#include "graphics/webgpu_context.h"

class Pipeline;

typedef std::variant<bool, int32_t, uint32_t, float> custom_define_type;

class Shader {

public:

	Shader();
	~Shader();

    bool load_from_file(const std::string& shader_path, const std::string& specialized_path = "", std::vector<std::string> define_specializations = {});
    bool load_from_source(const std::string& shader_source, const std::string& name, const std::string& specialized_path = "", std::vector<std::string> define_specializations = {});

	void reload();

	WGPUShaderModule get_module() const;

	void set_pipeline(Pipeline* pipeline);
	Pipeline* get_pipeline() { return pipeline_ref; }

	std::vector<WGPUBindGroupLayout>&    get_bind_group_layouts()    { return bind_group_layouts; }
	std::vector<WGPUVertexBufferLayout>& get_vertex_buffer_layouts() { return vertex_buffer_layouts; }

	std::string get_path() { return path; }

	bool is_loaded() { return loaded; }

    static void set_custom_define(const std::string &define_name, custom_define_type value);

private:

	void get_reflection_data(const std::string& shader_content);

    bool parse_preprocessor(std::string& shader_content, const std::string& shader_path);

    bool load(const std::string& shader_source, const std::string& specialized_name = "", std::vector<std::string> define_specializations = {});

	std::string path;
	std::string specialized_path;

	WGPUShaderModule shader_module = nullptr;

	std::vector<WGPUBindGroupLayout> bind_group_layouts;

	std::vector<WGPUVertexAttribute>	vertex_attributes;
	std::vector<WGPUVertexBufferLayout> vertex_buffer_layouts;

	// Pipeline that uses this shader
	Pipeline* pipeline_ref = nullptr;

    static std::unordered_map<std::string, const char*> engine_libraries;

    bool loaded_from_file = false;

    static std::unordered_map<std::string, custom_define_type> custom_defines;
    std::unordered_map<uint8_t, uint8_t> dynamic_bindings;
    std::vector<std::string> define_specializations;

	bool loaded = false;
};
