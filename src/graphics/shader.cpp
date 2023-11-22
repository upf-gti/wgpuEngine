#include "shader.h"

#include "utils.h"

#include <iostream>
#include <sstream>
#include <filesystem>

#include "pipeline.h"

#include "tint/tint.h"

#include "renderer_storage.h"

WebGPUContext* Shader::webgpu_context = nullptr;

Shader::~Shader()
{
	if (loaded) {
		wgpuShaderModuleRelease(shader_module);
	}

    for (auto& bind_group_layout : bind_group_layouts) {
        wgpuBindGroupLayoutRelease(bind_group_layout);
    }
}

bool Shader::load(const std::string& shader_path)
{
	path = shader_path;

	std::string shader_content;
	if (!read_file(path, shader_content))
		return false;

	std::istringstream f(shader_content);
	std::string line;

	std::string _directory = std::filesystem::path(path).parent_path().string();

	while (std::getline(f, line)) {

		auto tokens = tokenize(line);
		const std::string& tag = tokens[0];
		if (tag == "#include")
		{
			const std::string& include_name = tokens[1];
			const std::string& include_path = std::filesystem::relative(std::filesystem::path(_directory + "/" + include_name)).string();
			std::string new_content;
			if (!read_file(include_path, new_content)) {
                spdlog::error("Could not load shader include: {}", include_path);
				return false;
			}

            auto& library_references = RendererStorage::instance->shader_library_references;

			auto& references = library_references[include_path];

			if (!std::count(references.begin(), references.end(), path))
			{
				library_references[include_path].push_back(path);
			}

			//std::cout << " [" << include_name << "]";
			shader_content.replace(shader_content.find(tag), line.length() + 1, new_content);
		}
		// add other pres
		// else if (tag == "#...") { }
	}

    spdlog::info("Loading shader: {}", path);

	shader_module = webgpu_context->create_shader_module(shader_content.c_str());

	struct UserData {
		bool any_error = false;
	};
	UserData user_data;

	auto callback = [](WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const* compilation_info, void* p_user_data) {
		UserData& user_data = *reinterpret_cast<UserData*>(p_user_data);
		user_data.any_error = compilation_info->messageCount > 0;
	};

#ifndef __EMSCRIPTEN__
	wgpuShaderModuleGetCompilationInfo(shader_module, callback, &user_data);
#endif

	if (!user_data.any_error) {
		get_reflection_data(shader_path, shader_content);
		loaded = true;
	}
	else {
		loaded = false;
	}

	webgpu_context->print_errors();

	//std::cout << " [OK]" << std::endl;

	return loaded;
}

void Shader::get_reflection_data(const std::string& shader_path, const std::string& shader_content)
{
	tint::Source::File file(shader_path, shader_content);
    tint::wgsl::reader::Options parser_options;
    parser_options.allowed_features = tint::wgsl::AllowedFeatures::Everything();
	tint::Program tint_program = tint::wgsl::reader::Parse(&file, parser_options);
	tint::inspector::Inspector inspector(tint_program);

	std::map<int, std::map<int, WGPUBindGroupLayoutEntry>> entries_by_bind_group;

	using ResourceBinding = tint::inspector::ResourceBinding;

    uint8_t max_bind_group_index = 0;

	auto get_vertex_format_offset = [](WGPUVertexFormat format, uint16_t offset) -> WGPUVertexFormat {
		return static_cast<WGPUVertexFormat>(static_cast<int>(format + offset));
	};

	for (const auto& entry_point : inspector.GetEntryPoints()) {

		if (entry_point.stage == tint::inspector::PipelineStage::kVertex) {

			uint64_t offset = 0;

			for (const auto & input_variable : entry_point.input_variables) {
				WGPUVertexAttribute vertex_attribute = {};

				vertex_attribute.shaderLocation = input_variable.location_attribute;
				vertex_attribute.offset = offset;

				size_t byte_size = 0;
				switch (input_variable.component_type) {
				case tint::inspector::ComponentType::kF32:
					byte_size = sizeof(float);
					vertex_attribute.format = WGPUVertexFormat_Float32;
					break;
				case tint::inspector::ComponentType::kU32:
					byte_size = sizeof(uint32_t);
					vertex_attribute.format = WGPUVertexFormat_Uint32;
					break;
				case tint::inspector::ComponentType::kI32:
					byte_size = sizeof(int32_t);
					vertex_attribute.format = WGPUVertexFormat_Sint32;
					break;
				//case tint::inspector::ComponentType::kF16:
				//	byte_size = sizeof(uint16_t);
				//	break;
				default:
					std::cerr << "Shader reflection failed: Vertex component type not implemented" << std::endl;
					break;
				}

				switch (input_variable.composition_type) {
				case tint::inspector::CompositionType::kScalar:
					break;
				case tint::inspector::CompositionType::kVec2:
					byte_size *= 2;
					vertex_attribute.format = get_vertex_format_offset(vertex_attribute.format, 1);
					break;
				case tint::inspector::CompositionType::kVec3:
					byte_size *= 3;
					vertex_attribute.format = get_vertex_format_offset(vertex_attribute.format, 2);
					break;
				case tint::inspector::CompositionType::kVec4:
					byte_size *= 4;
					vertex_attribute.format = get_vertex_format_offset(vertex_attribute.format, 3);
					break;
				default:
					std::cerr << "Shader reflection failed: Vertex composition type not implemented" << std::endl;
					break;
				}

				offset += byte_size;
				
				vertex_attributes.push_back(vertex_attribute);
			}

			vertex_buffer_layouts.push_back(webgpu_context->create_vertex_buffer_layout(vertex_attributes, offset, WGPUVertexStepMode_Vertex));
		}

		bool has_sampler = false;

		for (const auto& resource_binding : inspector.GetResourceBindings(entry_point.name)) {

            if (max_bind_group_index < resource_binding.bind_group) {
                max_bind_group_index = resource_binding.bind_group;
            }

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
			case ResourceBinding::ResourceType::kSampler:
				has_sampler = true;
				entry.sampler.type = WGPUSamplerBindingType_Filtering;
				break;
			case ResourceBinding::ResourceType::kUniformBuffer:
				entry.buffer.type = WGPUBufferBindingType_Uniform;
				break;
			case ResourceBinding::ResourceType::kStorageBuffer:
				entry.buffer.type = WGPUBufferBindingType_Storage;
				break;
			case ResourceBinding::ResourceType::kReadOnlyStorageBuffer:
				entry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
				break;
			case ResourceBinding::ResourceType::kWriteOnlyStorageTexture:
				entry.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
				break;
#ifndef __EMSCRIPTEN__
            case ResourceBinding::ResourceType::kReadOnlyStorageTexture:
                entry.storageTexture.access = WGPUStorageTextureAccess_ReadOnly;
                break;
            case ResourceBinding::ResourceType::kReadWriteStorageTexture:
                entry.storageTexture.access = WGPUStorageTextureAccess_ReadWrite;
                break;
#endif
			default:
				std::cerr << "Shader reflection failed: Resource type not implemented" << std::endl;
				assert(0);
				break;
			}

			if (resource_binding.resource_type == ResourceBinding::ResourceType::kSampledTexture) {
				switch (resource_binding.sampled_kind)
				{
				case ResourceBinding::SampledKind::kFloat:
					if (has_sampler) {
						entry.texture.sampleType = WGPUTextureSampleType_Float;
					}
					else {
						entry.texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
					}
					break;
                case ResourceBinding::SampledKind::kUInt:
                    entry.texture.sampleType = WGPUTextureSampleType_Uint;
                    break;
				default:
					std::cerr << "Shader reflection failed: sample kind not implemented" << std::endl;
					assert(0);
					break;
				}

				switch (resource_binding.dim)
				{
				case ResourceBinding::TextureDimension::k1d:
					entry.texture.viewDimension = WGPUTextureViewDimension_1D;
					break;
				case ResourceBinding::TextureDimension::k2d:
					entry.texture.viewDimension = WGPUTextureViewDimension_2D;
					break;
				case ResourceBinding::TextureDimension::k3d:
					entry.texture.viewDimension = WGPUTextureViewDimension_3D;
					break;
				case ResourceBinding::TextureDimension::kCube:
					entry.texture.viewDimension = WGPUTextureViewDimension_Cube;
					break;
				default:
					std::cerr << "Shader reflection failed: view dimension not implemented" << std::endl;
					assert(0);
					break;
				}
			}

			if ((resource_binding.resource_type == ResourceBinding::ResourceType::kWriteOnlyStorageTexture) ||
                (resource_binding.resource_type == ResourceBinding::ResourceType::kReadWriteStorageTexture)) {
				switch (resource_binding.image_format)
				{
				case ResourceBinding::TexelFormat::kRgba8Unorm:
					entry.storageTexture.format = WGPUTextureFormat_RGBA8Unorm;
					break;
				case ResourceBinding::TexelFormat::kRgba16Float:
					entry.storageTexture.format = WGPUTextureFormat_RGBA16Float;
                    break;
                case ResourceBinding::TexelFormat::kR32Float:
                    entry.storageTexture.format = WGPUTextureFormat_R32Float;
                    break;
				case ResourceBinding::TexelFormat::kRgba32Float:
					entry.storageTexture.format = WGPUTextureFormat_RGBA32Float;
					break;
                case ResourceBinding::TexelFormat::kR32Uint:
                    entry.storageTexture.format = WGPUTextureFormat_R32Uint;
                    break;
				default:
					std::cerr << "Shader reflection failed: image format not implemented" << std::endl;
					assert(0);
					break;
				}

				switch (resource_binding.dim)
				{
				case ResourceBinding::TextureDimension::k1d:
					entry.storageTexture.viewDimension = WGPUTextureViewDimension_1D;
					break;
				case ResourceBinding::TextureDimension::k2d:
					entry.storageTexture.viewDimension = WGPUTextureViewDimension_2D;
					break;
				case ResourceBinding::TextureDimension::k3d:
					entry.storageTexture.viewDimension = WGPUTextureViewDimension_3D;
					break;
				case ResourceBinding::TextureDimension::kCube:
					entry.storageTexture.viewDimension = WGPUTextureViewDimension_Cube;
					break;
				default:
					std::cerr << "Shader reflection failed: storage view dimension not implemented" << std::endl;
					assert(0);
					break;
				}
			}
		}
	}

	for (int bind_group_index = 0; bind_group_index < max_bind_group_index + 1; ++bind_group_index) {

        auto& bind_group_entries = entries_by_bind_group[bind_group_index];

        if ((bind_group_index + 1) > bind_group_layouts.size()) {
            bind_group_layouts.resize(bind_group_index + 1);
        }

		std::vector<WGPUBindGroupLayoutEntry> entries;

		for (const auto& entry : bind_group_entries) {
			entries.push_back(entry.second);
		}

		bind_group_layouts[bind_group_index] = webgpu_context->create_bind_group_layout(entries);

		entries.clear();
	}
}

void Shader::reload()
{
	wgpuShaderModuleRelease(shader_module);

	bind_group_layouts.clear();
	vertex_attributes.clear();
	vertex_buffer_layouts.clear();

	load(path);

	if (pipeline_ref) {
		pipeline_ref->reload(this);
	}
}

WGPUShaderModule Shader::get_module() const
{
	return shader_module;
}

void Shader::set_pipeline(Pipeline* pipeline)
{
	pipeline_ref = pipeline;
}
