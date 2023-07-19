#include "shader.h"

#include "utils.h"

#include <iostream>
#include <sstream>

#include "pipeline.h"

#include "tint/tint.h"

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

	create_bind_group_layouts(shader_path, shader_content);

	loaded = true;
	std::cout << " [OK]" << std::endl;
}

void Shader::create_bind_group_layouts(const std::string& shader_path, const std::string& shader_content)
{

	tint::Source::File file(shader_path, shader_content);
	tint::Program tint_program = tint::reader::wgsl::Parse(&file);
	tint::inspector::Inspector inspector(&tint_program);

	std::map<int, std::map<int, WGPUBindGroupLayoutEntry>> entries_by_bind_group;

	using ResourceBinding = tint::inspector::ResourceBinding;

	for (const auto& entry_point : inspector.GetEntryPoints()) {

		for (const auto& resource_binding : inspector.GetResourceBindings(entry_point.name)) {

			// Creates new if didn't exist, otherwise return entry from previous entry_point
			WGPUBindGroupLayoutEntry& entry = entries_by_bind_group[resource_binding.bind_group][resource_binding.binding];

			// The binding index as used in the @binding attribute in the shader
			entry.binding = resource_binding.binding;

			// The stages that needs to access this resource
			switch (entry_point.stage)
			{
			case tint::inspector::PipelineStage::kVertex:
				entry.visibility |= WGPUShaderStage_Vertex;
				break;
			case tint::inspector::PipelineStage::kFragment:
				entry.visibility |= WGPUShaderStage_Fragment;
				break;
			case tint::inspector::PipelineStage::kCompute:
				entry.visibility |= WGPUShaderStage_Compute;
				break;
			default:
				break;
			}

			switch (resource_binding.resource_type)
			{
			case ResourceBinding::ResourceType::kSampledTexture:
				break;
			case ResourceBinding::ResourceType::kUniformBuffer:
				entry.buffer.type = WGPUBufferBindingType_Uniform;
				break;
			case ResourceBinding::ResourceType::kStorageBuffer:
				entry.buffer.type = WGPUBufferBindingType_Storage;
				break;
			case ResourceBinding::ResourceType::kWriteOnlyStorageTexture:
				entry.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
				break;
			default:
				std::cerr << "Shader reflection failed: Resource type not implemented" << std::endl;
				assert(0);
				break;
			}

			if (resource_binding.resource_type == ResourceBinding::ResourceType::kSampledTexture ||
				resource_binding.resource_type == ResourceBinding::ResourceType::kWriteOnlyStorageTexture) {
				switch (resource_binding.sampled_kind)
				{
				case ResourceBinding::SampledKind::kFloat:
					entry.texture.sampleType = WGPUTextureSampleType_Float;
					break;
				default:
					std::cerr << "Shader reflection failed: sample kind not implemented" << std::endl;
					assert(0);
					break;
				}

				switch (resource_binding.dim)
				{
				case ResourceBinding::TextureDimension::k2d:
					entry.texture.viewDimension = WGPUTextureViewDimension_2D;
					break;
				default:
					std::cerr << "Shader reflection failed: view dimension not implemented" << std::endl;
					assert(0);
					break;
				}
			}
		}
	}

	for (const auto& bind_group_entries : entries_by_bind_group) {
		std::vector<WGPUBindGroupLayoutEntry> entries;

		for (const auto& entry : bind_group_entries.second) {
			entries.push_back(entry.second);
		}

		bind_group_layouts.push_back(webgpu_context->create_bind_group_layout(entries));

		entries.clear();
	}
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
