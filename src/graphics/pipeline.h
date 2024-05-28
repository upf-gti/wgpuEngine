#pragma once

#include "webgpu_context.h"

#include <variant>
#include <unordered_map>

class Shader;
class Mesh;

struct PipelineDescription {

    WGPUCullMode cull_mode = WGPUCullMode_None;
    WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList;

    bool use_depth          = true;
    bool depth_read         = true;
    bool depth_write        = true;

    bool blending_enabled   = false;
    uint8_t sample_count    = 1;
};

class Pipeline {

public:

	~Pipeline();

	static WebGPUContext* webgpu_context;

    void create_render(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc = {});

	void create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout);
	void create_compute(Shader* shader);

	void reload(Shader* shader);

	void set(const WGPURenderPassEncoder& render_pass);
	void set(const WGPUComputePassEncoder& compute_pass);

    bool is_render_pipeline();
    bool is_compute_pipeline();

private:

	WGPUPipelineLayout pipeline_layout = nullptr;
	std::variant<std::monostate, WGPURenderPipeline, WGPUComputePipeline> pipeline;

	WGPUColorTargetState    color_target;
	WGPUBlendState const*   blend_state = nullptr;
    PipelineDescription     description;

	bool loaded = false;
};
