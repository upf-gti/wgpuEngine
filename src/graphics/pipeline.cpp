#include "pipeline.h"

#include "shader.h"
#include "mesh.h"

#include <iostream>

WebGPUContext* Pipeline::webgpu_context = nullptr;
std::vector<Pipeline*> Pipeline::registered_render_pipelines = {};
std::vector<Pipeline*> Pipeline::registered_compute_pipelines = {};

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

void Pipeline::create_render(Shader* shader, WGPUColorTargetState color_target, bool uses_depth_buffer, WGPUCullMode cull_mode, WGPUPrimitiveTopology topology)
{
	const std::vector<WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
	std::vector<WGPUBindGroupLayout> bind_group_layouts;

	for (const auto& bind_group_layout : layouts_by_id) {
		bind_group_layouts.push_back(bind_group_layout);
	}

    blending_enabled = color_target.blend != nullptr;

    this->topology = topology;
	this->color_target = color_target;

    if (blending_enabled) {
        this->blend_state = *color_target.blend;
    }
	this->uses_depth_buffer = uses_depth_buffer;

	pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);

	pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(), color_target, uses_depth_buffer, cull_mode, topology);

	shader->set_pipeline(this);
}

void Pipeline::create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout)
{
	this->pipeline_layout = pipeline_layout;
	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);
}

void Pipeline::create_compute(Shader* shader)
{
    const std::vector<WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
    std::vector<WGPUBindGroupLayout> bind_group_layouts;

	for (const auto& bind_group_layout : layouts_by_id) {
		bind_group_layouts.push_back(bind_group_layout);
	}

	pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);
	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);
}

void Pipeline::register_render_pipeline(Shader* shader, WGPUColorTargetState color_target, bool uses_depth_buffer, WGPUPrimitiveTopology topology)
{
    Pipeline* render_pipeline = new Pipeline();
    render_pipeline->create_render(shader, color_target, uses_depth_buffer, WGPUCullMode_None, topology);
    registered_render_pipelines.push_back(render_pipeline);
}

void Pipeline::register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout)
{
    Pipeline* compute_pipeline = new Pipeline();
    compute_pipeline->create_compute(shader, pipeline_layout);
    registered_compute_pipelines.push_back(compute_pipeline);
}

void Pipeline::clean_registered_pipelines()
{
    for (auto* pipeline : registered_render_pipelines) {
        delete pipeline;
    }

    for (auto* pipeline : registered_compute_pipelines) {
        delete pipeline;
    }

    registered_render_pipelines.clear();
    registered_compute_pipelines.clear();
}

void Pipeline::reload(Shader* shader)
{
	if (std::holds_alternative<WGPURenderPipeline>(pipeline)) {
		wgpuRenderPipelineRelease(std::get<WGPURenderPipeline>(pipeline));
        if (blending_enabled) {
            color_target.blend = &blend_state;
        }
		pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(),
            color_target, uses_depth_buffer, WGPUCullMode_None, topology);
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
