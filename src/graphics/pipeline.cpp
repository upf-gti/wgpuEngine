#include "pipeline.h"

#include "shader.h"

#include "spdlog/spdlog.h"

WebGPUContext* Pipeline::webgpu_context = nullptr;
std::unordered_map<RenderPipelineKey, Pipeline*> Pipeline::registered_render_pipelines = {};
std::unordered_map<Shader*, Pipeline*> Pipeline::registered_compute_pipelines = {};

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

void Pipeline::create_render(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc)
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

	pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(), p_color_target, desc.uses_depth_buffer, desc.uses_depth_write, desc.cull_mode, desc.topology);

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

    spdlog::info("Compiling pipeline for shader {}", shader->get_path());

	pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);
	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);
}

void Pipeline::register_render_pipeline(Shader* shader, const WGPUColorTargetState &p_color_target, const PipelineDescription& desc)
{
    RenderPipelineKey key = { shader, p_color_target, desc };

    if (registered_render_pipelines.contains(key)) {
        return;
    }

    Pipeline* render_pipeline = new Pipeline();
    render_pipeline->create_render(shader, p_color_target, desc);
    registered_render_pipelines[key] = render_pipeline;
}

void Pipeline::register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout)
{
    Pipeline* compute_pipeline = new Pipeline();
    compute_pipeline->create_compute(shader, pipeline_layout);
    registered_compute_pipelines[shader] = compute_pipeline;
}

void Pipeline::clean_registered_pipelines()
{
    for (auto [key, pipeline] : registered_render_pipelines) {
        delete pipeline;
    }

    for (auto [shader, pipeline] : registered_compute_pipelines) {
        delete pipeline;
    }

    registered_render_pipelines.clear();
    registered_compute_pipelines.clear();
}

void Pipeline::reload(Shader* shader)
{
	if (std::holds_alternative<WGPURenderPipeline>(pipeline)) {
		wgpuRenderPipelineRelease(std::get<WGPURenderPipeline>(pipeline));
        if (description.blending_enabled) {
            color_target.blend = blend_state;
        }
		pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(),
            color_target, description.uses_depth_buffer, description.uses_depth_write, description.cull_mode, description.topology);
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
