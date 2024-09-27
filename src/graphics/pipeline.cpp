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
    if (pipeline_layout) {
        wgpuPipelineLayoutRelease(pipeline_layout);
    }

    if (std::holds_alternative<WGPURenderPipeline>(pipeline)) {
        wgpuRenderPipelineRelease(std::get<WGPURenderPipeline>(pipeline));
    }
    else if (std::holds_alternative<WGPUComputePipeline>(pipeline)) {
        wgpuComputePipelineRelease(std::get<WGPUComputePipeline>(pipeline));
    }
}

void render_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPURenderPipeline pipeline, char const* message, void* userdata)
{
    Pipeline* render_pipeline = static_cast<Pipeline*>(userdata);
    if (status == WGPUCreatePipelineAsyncStatus_Success) {
        render_pipeline->pipeline = pipeline;
        render_pipeline->loaded = true;
    }
}

void compute_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline, char const* message, void* userdata)
{
    Pipeline* compute_pipeline = static_cast<Pipeline*>(userdata);
    if (status == WGPUCreatePipelineAsyncStatus_Success) {
        compute_pipeline->pipeline = pipeline;
        compute_pipeline->loaded = true;
    }
}

void Pipeline::create_render_common(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc)
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

    pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);
}

void Pipeline::create_compute_common(Shader* shader)
{
    const std::vector<WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
    std::vector<WGPUBindGroupLayout> bind_group_layouts;

    for (const auto& bind_group_layout : layouts_by_id) {
        bind_group_layouts.push_back(bind_group_layout);
    }

    pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);
}

void Pipeline::create_render(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc)
{
    create_render_common(shader, p_color_target, desc);

    spdlog::info("Compiling render pipeline for shader {}", shader->get_path());

	pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(), p_color_target, desc);

	shader->set_pipeline(this);

    loaded = true;
}

void Pipeline::create_render_async(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc)
{
    create_render_common(shader, p_color_target, desc);

    spdlog::info("Compiling async render pipeline for shader {}", shader->get_path());

    webgpu_context->create_render_pipeline_async(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(), p_color_target, render_pipeline_creation_callback, (void*)this, desc);

    shader->set_pipeline(this);

    async_compile = true;
}

void Pipeline::create_compute(Shader* shader)
{
    create_compute_common(shader);

    spdlog::info("Compiling compute pipeline for shader {}", shader->get_path());

	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);

    loaded = true;
}

void Pipeline::create_compute_async(Shader* shader)
{
    create_compute_common(shader);

    spdlog::info("Compiling async compute pipeline for shader {}", shader->get_path());

    webgpu_context->create_compute_pipeline_async(shader->get_module(), pipeline_layout, compute_pipeline_creation_callback, (void*)this);

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

		pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(),
            color_target, description);
	}
	else {
		wgpuComputePipelineRelease(std::get<WGPUComputePipeline>(pipeline));
		pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);
	}
}

void Pipeline::set(const WGPURenderPassEncoder& render_pass) const
{
    if (async_compile) {
        while (!loaded) {
            webgpu_context->process_events();
        }
    }

	wgpuRenderPassEncoderSetPipeline(render_pass, std::get<WGPURenderPipeline>(pipeline));
}

void Pipeline::set(const WGPUComputePassEncoder& compute_pass) const
{
    if (async_compile) {
        while (!loaded) {
            webgpu_context->process_events();
        }
    }

	wgpuComputePassEncoderSetPipeline(compute_pass, std::get<WGPUComputePipeline>(pipeline));
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
