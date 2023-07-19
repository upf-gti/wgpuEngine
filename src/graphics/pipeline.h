#pragma once

#include <webgpu/webgpu.h>

#include "webgpu_context.h"

#include <variant>

class Shader;

class Pipeline {

public:

	~Pipeline();

	static WebGPUContext* webgpu_context;

	void create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout);
	void create_compute(Shader* shader);

	void reload(const Shader* shader);

	void set(const WGPURenderPassEncoder& render_pass);
	void set(const WGPUComputePassEncoder& compute_pass);

private:

	WGPUPipelineLayout pipeline_layout;
	std::variant< WGPURenderPipeline, WGPUComputePipeline> pipeline;

	bool loaded = false;
};
