#pragma once

#include <webgpu/webgpu.h>

#include "webgpu_context.h"

#include <variant>
#include <unordered_set>

class Shader;
class Mesh;

class Pipeline {

public:

	~Pipeline();

	static WebGPUContext* webgpu_context;

	void create_render(Shader* shader, WGPUColorTargetState color_target, bool uses_depth_buffer = false, WGPUCullMode cull_mode = WGPUCullMode_None, WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList);

	void create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout);
	void create_compute(Shader* shader);

    static void register_render_pipeline(Shader* shader, WGPUColorTargetState color_target, bool uses_depth_buffer = false, WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList);
    static void register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout);
    static void clean_registered_pipelines();

	void reload(Shader* shader);

	void set(const WGPURenderPassEncoder& render_pass);
	void set(const WGPUComputePassEncoder& compute_pass);

private:

	WGPUPipelineLayout pipeline_layout = nullptr;
	std::variant<std::monostate, WGPURenderPipeline, WGPUComputePipeline> pipeline;

    static std::vector<Pipeline*> registered_render_pipelines;
    static std::vector<Pipeline*> registered_compute_pipelines;

	WGPUColorTargetState color_target;
	WGPUBlendState		 blend_state;

	bool uses_depth_buffer = false;
    bool blending_enabled = false;

	bool loaded = false;
};
