#pragma once

#include <webgpu/webgpu.h>

#include "webgpu_context.h"

#include <variant>

class Shader;
class EntityMesh;

class Pipeline {

public:

	~Pipeline();

	static WebGPUContext* webgpu_context;

	void create_render(Shader* shader, WGPUColorTargetState color_target, bool uses_depth_buffer = false);

	void create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout);
	void create_compute(Shader* shader);

	void reload(Shader* shader);

	void set(const WGPURenderPassEncoder& render_pass);
	void set(const WGPUComputePassEncoder& compute_pass);

	void add_renderable(EntityMesh* entity);
	void clean_renderables();

	const std::vector<EntityMesh*>& get_render_list() { return render_list; }

private:

	WGPUPipelineLayout pipeline_layout;
	std::variant< WGPURenderPipeline, WGPUComputePipeline> pipeline;

	// Entities to be rendered this frame
	std::vector<EntityMesh*> render_list;

	WGPUColorTargetState color_target;
	WGPUBlendState		 blend_state;

	bool uses_depth_buffer = false;

	bool loaded = false;
};
