#include "pipeline.h"

#include "shader.h"
#include "mesh.h"

#include <iostream>

WebGPUContext* Pipeline::webgpu_context = nullptr;
std::vector<Pipeline*> Pipeline::registered_render_pipelines = {};
std::vector<Pipeline*> Pipeline::registered_compute_pipelines = {};

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

	this->color_target = color_target;
	this->blend_state = *color_target.blend;
	this->uses_depth_buffer = uses_depth_buffer;

	pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);

	pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(), color_target, uses_depth_buffer);

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
	const std::map<int, WGPUBindGroupLayout> layouts_by_id = shader->get_bind_group_layouts();
	std::vector<WGPUBindGroupLayout> bind_group_layouts;

	for (const auto& bind_group_layout : layouts_by_id) {
		bind_group_layouts.push_back(bind_group_layout.second);
	}

	pipeline_layout = webgpu_context->create_pipeline_layout(bind_group_layouts);
	pipeline = webgpu_context->create_compute_pipeline(shader->get_module(), pipeline_layout);

	shader->set_pipeline(this);
}

void Pipeline::register_render_pipeline(Shader* shader, WGPUColorTargetState color_target, bool uses_depth_buffer)
{
    Pipeline* render_pipeline = new Pipeline();
    render_pipeline->create_render(shader, color_target, uses_depth_buffer);
    registered_render_pipelines.push_back(render_pipeline);
}

void Pipeline::register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout)
{
    Pipeline* compute_pipeline = new Pipeline();
    compute_pipeline->create_compute(shader, pipeline_layout);
    registered_compute_pipelines.push_back(compute_pipeline);
}

void Pipeline::clean_registered_pipelines_renderables()
{
    for (auto* pipeline : registered_render_pipelines) {
        pipeline->clean_renderables();
    }
}

void Pipeline::render_registered_pipelines_renderables(const WGPURenderPassEncoder& render_pass, const WGPUBindGroup& render_bind_group_camera)
{
    for (auto* pipeline : registered_render_pipelines) {

        // Bind Pipeline
        pipeline->set(render_pass);

        for (const auto mesh : pipeline->get_render_list()) {

            // Not initialized
            if (mesh->get_vertex_count() == 0) {
                std::cerr << "Skipping not initialized mesh" << std::endl;
                continue;
            }

            mesh->update_instance_model_matrices();

            // Set bind group
            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, mesh->get_bind_group(), 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 1, render_bind_group_camera, 0, nullptr);

            //ui::Widget* widget = ui::Controller::get(mesh->get_alias());
            //if (widget)
            //{
            //    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(widget->uniforms.data), 0, &widget->ui_data, sizeof(ui::sUIData));
            //    wgpuRenderPassEncoderSetBindGroup(render_pass, 2, widget->bind_group, 0, nullptr);
            //}

            // Set vertex buffer while encoding the render pass
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, mesh->get_vertex_buffer(), 0, mesh->get_byte_size());

            // Submit drawcall
            wgpuRenderPassEncoderDraw(render_pass, mesh->get_vertex_count(), mesh->get_instances_size(), 0, 0);
        }
    }
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
		color_target.blend = &blend_state;
		pipeline = webgpu_context->create_render_pipeline(shader->get_module(), pipeline_layout, shader->get_vertex_buffer_layouts(), color_target, uses_depth_buffer);
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

void Pipeline::add_renderable(Mesh* mesh)
{
	render_list.insert(mesh);
}

void Pipeline::clean_renderables()
{
	for (const auto mesh: render_list) {
		mesh->clear_instances();
	}

	render_list.clear();
}
