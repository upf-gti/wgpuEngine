#pragma once

#include "webgpu_context.h"

#include <variant>
#include <unordered_map>

class Shader;
class Mesh;

using WGPUPipeline = std::variant<std::monostate, WGPURenderPipeline, WGPUComputePipeline>;

class Pipeline {

public:

	~Pipeline();

	static WebGPUContext* webgpu_context;

    void create_render(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc = {});
    void create_render_async(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc = {});

	//void create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout);
	void create_compute(Shader* shader);

    //void create_compute_async(Shader* shader, WGPUPipelineLayout pipeline_layout);
    void create_compute_async(Shader* shader);

	void reload(Shader* shader);

	void set(const WGPURenderPassEncoder& render_pass) const;
	void set(const WGPUComputePassEncoder& compute_pass) const;

    bool is_render_pipeline() const;
    bool is_compute_pipeline() const;

    bool is_msaa_allowed() const;

    friend void render_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPURenderPipeline pipeline, char const* message, void* userdata);
    friend void compute_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline, char const* message, void* userdata);

private:

    void create_render_common(Shader* shader, const WGPUColorTargetState& p_color_target, const PipelineDescription& desc = {});
    void create_compute_common(Shader* shader);

	WGPUPipelineLayout pipeline_layout = nullptr;
    WGPUPipeline pipeline;

	WGPUColorTargetState    color_target;
	WGPUBlendState const*   blend_state = nullptr;
    PipelineDescription     description;

    bool async_compile = false;
	bool loaded = false;
};
