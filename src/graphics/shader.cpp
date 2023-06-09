#include "shader.h"

#include "utils.h"

#include <iostream>

std::map<std::string, Shader*> Shader::shaders;
WebGPUContext* Shader::webgpu_context = nullptr;

Shader::~Shader()
{
	if (loaded) {
		wgpuShaderModuleRelease(shader_module);
	}
}

void Shader::load(const std::string& shader_path, const std::vector<std::string>& libraries)
{
	path = shader_path;

	std::cout << "Loading shader: " << shader_path << std::endl;

	std::string libraries_content;
	for (const auto& library_path : libraries) {
		std::string library_content;
		if (!readFile(library_path, library_content)) {
			std::cerr << "Could not load shader library: " << library_path << std::endl;
			return;
		}

		libraries_content += library_content;
	}

	std::string shader_content;
	if (!readFile(shader_path, shader_content))
		return;

	shader_content = libraries_content + shader_content;

	shader_module = webgpu_context->create_shader_module(shader_content.c_str());
	loaded = true;
}

Shader* Shader::get(const std::string& shader_path, const std::vector<std::string>& libraries)
{
	std::string name;

	// check if already loaded
	std::map<std::string, Shader*>::iterator it = shaders.find(shader_path);
	if (it != shaders.end())
		return it->second;

	Shader* sh = new Shader();
	sh->load(shader_path, libraries);

	// register in map
	shaders[name] = sh;
	
	return sh;
}

WGPUShaderModule& Shader::get_module()
{
	return shader_module;
}
