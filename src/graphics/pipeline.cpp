#include "pipeline.h"

#include "renderer.h"
#include "shader.h"

#include "spdlog/spdlog.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

WebGPUContext* Pipeline::webgpu_context = nullptr;

Pipeline::~Pipeline()
{
    if (std::holds_alternative<WGPURenderPipeline>(pipeline)) {
        wgpuRenderPipelineRelease(std::get<WGPURenderPipeline>(pipeline));
    }
    else if (std::holds_alternative<WGPUComputePipeline>(pipeline)) {
        wgpuComputePipelineRelease(std::get<WGPUComputePipeline>(pipeline));
    }
}

void render_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPURenderPipeline pipeline, char const* message, void* userdata1, void* userdata2)
{
    Pipeline* render_pipeline = static_cast<Pipeline*>(userdata1);
    if (status == WGPUCreatePipelineAsyncStatus_Success) {
        render_pipeline->pipeline = pipeline;
        render_pipeline->loaded = true;
    }
}

void compute_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline, char const* message, void* userdata1, void* userdata2)
{
    Pipeline* compute_pipeline = static_cast<Pipeline*>(userdata1);
    if (status == WGPUCreatePipelineAsyncStatus_Success) {
        compute_pipeline->pipeline = pipeline;
        compute_pipeline->loaded = true;
    }
}

void Pipeline::create_render_common(Shader* shader, const WGPUColorTargetState& p_color_target, const RenderPipelineDescription& desc)
{
    const std::vector<WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
    std::vector<WGPUBindGroupLayout> bind_group_layouts;

    for (const auto& bind_group_layout : layouts_by_id) {
        bind_group_layouts.push_back(bind_group_layout);
    }

    // Copy pipeline info
    description = desc;
    description.blending_enabled = (p_color_target.blend != nullptr);
    color_target = p_color_target;

    if (description.blending_enabled) {
        blend_state = p_color_target.blend;
    }
}

void Pipeline::create_compute_common(Shader* shader)
{
    const std::vector<WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
    std::vector<WGPUBindGroupLayout> bind_group_layouts;

    for (const auto& bind_group_layout : layouts_by_id) {
        bind_group_layouts.push_back(bind_group_layout);
    }
}

void Pipeline::create_render(Shader* shader, const WGPUColorTargetState& p_color_target, const RenderPipelineDescription& desc, const std::vector<WGPUConstantEntry> &constants)
{
    create_render_common(shader, p_color_target, desc);

    spdlog::info("Compiling render pipeline for shader {}", shader->get_path());

	pipeline = webgpu_context->create_render_pipeline(shader->get_module(), shader->get_pipeline_layout(), shader->get_vertex_buffer_layouts(), p_color_target, desc, constants);

	shader->set_pipeline(this);

    loaded = true;
}

void Pipeline::create_render_async(Shader* shader, const WGPUColorTargetState& p_color_target, const RenderPipelineDescription& desc, const std::vector<WGPUConstantEntry> &constants)
{
    create_render_common(shader, p_color_target, desc);

    spdlog::info("Compiling async render pipeline for shader {}", shader->get_path());

    WGPUCreateRenderPipelineAsyncCallbackInfo2 callback_info = {};
    callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
    callback_info.callback = render_pipeline_creation_callback;
    callback_info.userdata1 = (void*)this;

    webgpu_context->create_render_pipeline_async(shader->get_module(), shader->get_pipeline_layout(), shader->get_vertex_buffer_layouts(),
        p_color_target, callback_info, desc, constants);

    shader->set_pipeline(this);

    async_compile = true;
}

void Pipeline::create_compute(Shader* shader, const char* entry_point, const std::vector<WGPUConstantEntry> &constants)
{
    create_compute_common(shader);

    spdlog::info("Compiling compute pipeline for shader {}", shader->get_path());

	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), shader->get_pipeline_layout(), entry_point, constants);

	shader->set_pipeline(this);

    loaded = true;
}

void Pipeline::create_compute_async(Shader* shader, const char* entry_point, const std::vector<WGPUConstantEntry> &constants)
{
    create_compute_common(shader);

    spdlog::info("Compiling async compute pipeline for shader {}", shader->get_path());

    WGPUCreateComputePipelineAsyncCallbackInfo2 callback_info = {};
    callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
    callback_info.callback = compute_pipeline_creation_callback;
    callback_info.userdata1 = (void*)this;

    webgpu_context->create_compute_pipeline_async(shader->get_module(), shader->get_pipeline_layout(), callback_info, entry_point, constants);

    shader->set_pipeline(this);

    async_compile = true;
}

void Pipeline::reload(Shader* shader)
{
	if (std::holds_alternative<WGPURenderPipeline>(pipeline)) {
		wgpuRenderPipelineRelease(std::get<WGPURenderPipeline>(pipeline));
        if (description.blending_enabled) {
            color_target.blend = blend_state;
        }

        description.sample_count = Renderer::instance->get_msaa_count();

		pipeline = webgpu_context->create_render_pipeline(shader->get_module(), shader->get_pipeline_layout(), shader->get_vertex_buffer_layouts(),
            color_target, description);
	}
	else {
		wgpuComputePipelineRelease(std::get<WGPUComputePipeline>(pipeline));
		pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), shader->get_pipeline_layout());
	}
}

bool Pipeline::set(const WGPURenderPassEncoder& render_pass) const
{
    if (async_compile && !loaded) {
        return false;
    }

	wgpuRenderPassEncoderSetPipeline(render_pass, std::get<WGPURenderPipeline>(pipeline));

    return true;
}

bool Pipeline::set(const WGPUComputePassEncoder& compute_pass) const
{
    if (async_compile && !loaded) {
        return false;
    }

	wgpuComputePassEncoderSetPipeline(compute_pass, std::get<WGPUComputePipeline>(pipeline));

    return true;
}

bool Pipeline::is_render_pipeline() const
{
    return std::holds_alternative<WGPURenderPipeline>(pipeline);
}

bool Pipeline::is_compute_pipeline() const
{
    return std::holds_alternative<WGPUComputePipeline>(pipeline);
}

bool Pipeline::is_msaa_allowed() const
{
    return description.allow_msaa;
}
