#include "pipeline.h"

#include "shader.h"

WebGPUContext* Pipeline::webgpu_context = nullptr;

Pipeline::~Pipeline()
{
}

void Pipeline::create_render(Shader* shader, WGPUColorTargetState color_target, bool uses_depth_buffer)
{
	const std::map<int, WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
	std::vector<WGPUBindGroupLayout> bind_group_layouts;

	for (const auto& bind_group_layout : layouts_by_id) {
		bind_group_layouts.push_back(bind_group_layout.second);
	}

	pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);

	pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(), color_target, uses_depth_buffer);
}

void Pipeline::create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout)
{
	this->pipeline_layout = pipeline_layout;
	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);
}

void Pipeline::create_compute(Shader* shader)
{
	const std::map<int, WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
	std::vector<WGPUBindGroupLayout> bind_group_layouts;

	for (const auto& bind_group_layout : layouts_by_id) {
		bind_group_layouts.push_back(bind_group_layout.second);
	}

	pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);
	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);
}

void Pipeline::reload(const Shader* shader)
{
	if (std::holds_alternative<WGPURenderPipeline>(pipeline)) {
		wgpuRenderPipelineRelease(std::get<WGPURenderPipeline>(pipeline));
		//pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout);
	}
	else {
		wgpuComputePipelineRelease(std::get<WGPUComputePipeline>(pipeline));
		pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);
	}
}

void Pipeline::set(const WGPURenderPassEncoder& render_pass)
{
	wgpuRenderPassEncoderSetPipeline(render_pass, std::get<WGPURenderPipeline>(pipeline));
}

void Pipeline::set(const WGPUComputePassEncoder& compute_pass)
{
	wgpuComputePassEncoderSetPipeline(compute_pass, std::get<WGPUComputePipeline>(pipeline));
}
