#include "shader.h"

#include "utils.h"

#include <iostream>
#include <sstream>

#include "pipeline.h"

std::map<std::string, Shader*> Shader::shaders;
WebGPUContext* Shader::webgpu_context = nullptr;

Shader::~Shader()
{
	if (loaded) {
		wgpuShaderModuleRelease(shader_module);
	}
}

void Shader::load(const std::string& shader_path)
{
	path = shader_path;

	std::cout << "Loading shader: " << path;

	std::string shader_content;
	if (!read_file(path, shader_content))
		return;

	std::istringstream f(shader_content);
	std::string include_content;
	std::string line;

	size_t ix = path.find_last_of('/');
	std::string _directory = path.substr(0, ix + 1);

	while (std::getline(f, line)) {

		auto tokens = tokenize(line);
		const std::string& tag = tokens[0];
		if (tag == "#include")
		{
			const std::string& include_name = tokens[1];
			const std::string& include_path = _directory + include_name;
			std::string new_content;
			if (!read_file(include_path, new_content)) {
				std::cerr << "Could not load shader include: " << include_path << std::endl;
				return;
			}

			std::cout << " [" << include_name << "]";
			shader_content.replace(shader_content.find(tag), line.length() + 1, "");
			include_content += new_content;
		}
		// add other pres
		// else if (tag == "#...") { }
	}

	shader_content = include_content + shader_content;

	shader_module = webgpu_context->create_shader_module(shader_content.c_str());

	loaded = true;
	std::cout << " [OK]" << std::endl;
}

void Shader::reload()
{
	wgpuShaderModuleRelease(shader_module);

	load(path);

	if (pipeline_ref) {
		pipeline_ref->reload(this);
	}
}

Shader* Shader::get(const std::string& shader_path)
{
	std::string name = shader_path;

	// check if already loaded
	std::map<std::string, Shader*>::iterator it = shaders.find(shader_path);
	if (it != shaders.end())
		return it->second;

	Shader* sh = new Shader();
	sh->load(shader_path);

	// register in map
	shaders[name] = sh;
	
	return sh;
}

WGPUShaderModule Shader::get_module() const
{
	return shader_module;
}

void Shader::set_pipeline(Pipeline* pipeline)
{
	pipeline_ref = pipeline;
}
