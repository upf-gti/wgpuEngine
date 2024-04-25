#include "shader.h"

#include "pipeline.h"

#define TINT_BUILD_WGSL_READER 1
#include "src/tint/lang/wgsl/reader/reader.h"
#include "src/tint/lang/wgsl/inspector/inspector.h"

#include "renderer_storage.h"
#include "renderer.h"

#include "framework/utils/utils.h"

#include "spdlog/spdlog.h"

std::unordered_map<std::string, custom_define_type> Shader::custom_defines;

Shader::~Shader()
{
	if (loaded) {
		wgpuShaderModuleRelease(shader_module);
	}

    for (auto& bind_group_layout : bind_group_layouts) {
        wgpuBindGroupLayoutRelease(bind_group_layout);
    }
}

bool Shader::load_from_file(const std::string& shader_path, const std::string& specialized_path, std::vector<std::string> define_specializations)
{
	path = shader_path;

    if (!specialized_path.empty()) {
        this->specialized_path = specialized_path;
    }
    else {
        this->specialized_path = path;
    }

    if (!define_specializations.empty()) {
        this->define_specializations = define_specializations;
    }

    spdlog::info("Loading shader: {}", path);

	std::string shader_content;
    if (!read_file(path, shader_content)) {
        spdlog::error("\tError reading shader");
		return false;
    }

	return load(shader_content, specialized_path, define_specializations);
}

bool Shader::load_from_source(const std::string& shader_source, const std::string& name, const std::string& specialized_name, std::vector<std::string> define_specializations)
{
    path = name;

    if (!specialized_path.empty()) {
        this->specialized_path = specialized_path;
    }
    else {
        this->specialized_path = path;
    }

    if (!define_specializations.empty()) {
        this->define_specializations = define_specializations;
    }

    return load(shader_source, specialized_path, define_specializations);
}

bool Shader::parse_preprocessor(std::string &shader_content, const std::string &shader_path)
{
    std::istringstream string_stream(shader_content);
    std::string line;

    std::string _directory = dirname_of_file(shader_path);

    std::streampos line_pos;
    while (std::getline(string_stream, line)) {

        auto tokens = tokenize(line);
        const std::string& tag = tokens[0];

        if (tag == "#include")
        {
            const std::string& include_name = tokens[1];
            const std::string& include_path = _directory + "/" + include_name;
            std::string new_content;

            if (!read_file(include_path, new_content)) {
                spdlog::error("\tCould not load shader include: {}", include_path);
                return false;
            }

            if (!parse_preprocessor(new_content, include_path)) {
                return false;
            }

            auto& library_references = RendererStorage::instance->shader_library_references;

            auto& references = library_references[include_path];

            if (!std::count(references.begin(), references.end(), specialized_path))
            {
                library_references[include_path].push_back(specialized_path);
            }

            //std::cout << " [" << include_name << "]";
            shader_content.replace(line_pos, line.length(), new_content);
            //spdlog::info(shader_content);

            line_pos += new_content.length();
        }
        // add other pres
        else if (tag == "#define") {
            const std::string& define_name = tokens[1];

            std::string final_value;

            Renderer* renderer = Renderer::instance;
          
            if (define_name == "GAMMA_CORRECTION") {
                final_value = renderer->get_openxr_available() ? "0" : "1";
            }

            for (const auto define : custom_defines) {
                if (define_name == define.first) {
                    if (std::holds_alternative<bool>(define.second)) {
                        final_value = std::get<bool>(define.second) ? "1" : "0";
                    }
                    else
                        if (std::holds_alternative<int32_t>(define.second)) {
                            final_value = std::to_string(std::get<int32_t>(define.second));
                        }
                        else
                            if (std::holds_alternative<uint32_t>(define.second)) {
                                final_value = std::to_string(std::get<uint32_t>(define.second));
                            }
                            else
                                if (std::holds_alternative<float>(define.second)) {
                                    final_value = std::to_string(std::get<float>(define.second));
                                }
                }
            }

            std::string new_value = "const " + define_name + " = " + final_value + ";";
            shader_content.replace(line_pos, line.length(), new_value);

            line_pos += new_value.length();
        }
        else if (tag == "#dynamic") {

            // convert to int, sorry :(
            uint8_t group = tokens[1].at(7) - '0';
            uint8_t binding = tokens[2].at(9) - '0';

            dynamic_bindings[group] = binding;

            shader_content.replace(line_pos, tag.length() + 1, "");

            line_pos += line.length() - (tag.length() + 1);
        }
        else if (tag == "#ifdef") {

            // remove #ifdef line
            shader_content.replace(line_pos, line.length() + 1, "");

            // if specialization is defined
            if (std::find(define_specializations.begin(), define_specializations.end(), tokens[1]) != define_specializations.end()) {

                // Just advance, do nothing
                std::string tag_found = continue_until_tags(string_stream, line_pos, line, { "#endif", "#else" });

                // delete #else condition
                if (tag_found == "#else") {
                    // remove #else line
                    shader_content.replace(line_pos, line.length() + 1, "");

                    delete_until_tags(string_stream, shader_content, line_pos, line, { "#endif" });
                }
            }
            else {

                // Delete all lines in between
                std::string tag_found = delete_until_tags(string_stream, shader_content, line_pos, line, { "#endif", "#else" });

                // mantain #else condition
                if (tag_found == "#else") {
                    // remove #else line
                    shader_content.replace(line_pos, line.length() + 1, "");

                    continue_until_tags(string_stream, line_pos, line, { "#endif" });
                }
            }

            // remove #endif line
            shader_content.replace(line_pos, line.length(), "");
        }
        else {
            line_pos += line.length();
        }

        // Account for \n
        line_pos += 1;
    }

    return true;
}

bool Shader::load(const std::string& shader_source, const std::string& specialized_name, std::vector<std::string> define_specializations)
{
    std::string shader_source_processed = shader_source;

    if (!parse_preprocessor(shader_source_processed, path)) {
        spdlog::error("\tPreprocessor parsing error");
        return false;
    }

    for (const std::string& specialization : define_specializations) {
        spdlog::trace("\t{}", specialization);
    }

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    shader_module = webgpu_context->create_shader_module(shader_source_processed.c_str());

    struct UserData {
        bool any_error = false;
    } user_data;

    auto callback = [](WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const* compilation_info, void* p_user_data) {
        UserData& user_data = *reinterpret_cast<UserData*>(p_user_data);
        user_data.any_error = compilation_info->messageCount > 0;
    };

#ifndef __EMSCRIPTEN__
    wgpuShaderModuleGetCompilationInfo(shader_module, callback, &user_data);
#endif

    if (!user_data.any_error) {
        get_reflection_data(shader_source_processed);
        loaded = true;
    }
    else {
        loaded = false;
    }

    webgpu_context->process_events();

    return loaded;
}

void Shader::set_custom_define(const std::string& define_name, custom_define_type value)
{
    custom_defines[define_name] = value;
}

void Shader::get_reflection_data(const std::string& shader_content)
{
	tint::Source::File file("", shader_content);
    tint::wgsl::reader::Options parser_options;
    parser_options.allowed_features = tint::wgsl::AllowedFeatures::Everything();
	tint::Program tint_program = tint::wgsl::reader::Parse(&file, parser_options);
	tint::inspector::Inspector inspector(tint_program);

	std::map<int, std::map<int, WGPUBindGroupLayoutEntry>> entries_by_bind_group;

	using ResourceBinding = tint::inspector::ResourceBinding;

    uint8_t max_bind_group_index = 0;

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

	auto get_vertex_format_offset = [](WGPUVertexFormat format, uint16_t offset) -> WGPUVertexFormat {
		return static_cast<WGPUVertexFormat>(static_cast<int>(format + offset));
	};

	for (const auto& entry_point : inspector.GetEntryPoints()) {

		if (entry_point.stage == tint::inspector::PipelineStage::kVertex) {

			uint64_t offset = 0;

			for (const auto & input_variable : entry_point.input_variables) {
				WGPUVertexAttribute vertex_attribute = {};

				vertex_attribute.shaderLocation = input_variable.attributes.location.value();
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
					spdlog::error("Shader reflection failed: Vertex component type not implemented");
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
                    spdlog::error("Shader reflection failed: Vertex composition type not implemented");
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

            if (dynamic_bindings.contains(resource_binding.bind_group)) {
                if (entry.binding == dynamic_bindings[resource_binding.bind_group]) {
                    entry.buffer.hasDynamicOffset = true;
                }
            }

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
            case ResourceBinding::ResourceType::kReadOnlyStorageTexture:
                entry.storageTexture.access = WGPUStorageTextureAccess_ReadOnly;
                break;
            case ResourceBinding::ResourceType::kReadWriteStorageTexture:
                entry.storageTexture.access = WGPUStorageTextureAccess_ReadWrite;
                break;
			default:
                spdlog::error("Shader reflection failed: Resource type not implemented");
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
                    spdlog::error("Shader reflection failed: sample kind not implemented");
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
                    spdlog::error("Shader reflection failed: view dimension not implemented");
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
                case ResourceBinding::TexelFormat::kRgba32Float:
                    entry.storageTexture.format = WGPUTextureFormat_RGBA32Float;
                    break;
                case ResourceBinding::TexelFormat::kR32Float:
                    entry.storageTexture.format = WGPUTextureFormat_R32Float;
                    break;
                case ResourceBinding::TexelFormat::kR32Uint:
                    entry.storageTexture.format = WGPUTextureFormat_R32Uint;
                    break;
                case ResourceBinding::TexelFormat::kRg32Float:
                    entry.storageTexture.format = WGPUTextureFormat_RG32Float;
                    break;
				default:
                    spdlog::error("Shader reflection failed: image format not implemented");
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
                case ResourceBinding::TextureDimension::k2dArray:
                    entry.storageTexture.viewDimension = WGPUTextureViewDimension_2DArray;
                    break;
				case ResourceBinding::TextureDimension::k3d:
					entry.storageTexture.viewDimension = WGPUTextureViewDimension_3D;
					break;
				case ResourceBinding::TextureDimension::kCube:
					entry.storageTexture.viewDimension = WGPUTextureViewDimension_Cube;
					break;
				default:
                    spdlog::error("Shader reflection failed: storage view dimension not implemented");
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

	load_from_file(path, specialized_path, define_specializations);

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
