#include "pipeline.h"

#include "shader.h"

WebGPUContext* Pipeline::webgpu_context = nullptr;

Pipeline::~Pipeline()
{
}

void Pipeline::create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout)
{
	this->pipeline_layout = pipeline_layout;
	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);
}

void Pipeline::reload(const Shader* shader)
{
	if (std::holds_alternative<WGPURenderPipeline>(pipeline)) {
		wgpuRenderPipelineRelease(std::get<WGPURenderPipeline>(pipeline));
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
