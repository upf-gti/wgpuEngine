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

    void create_render(Shader* shader, const WGPUColorTargetState& p_color_target, const RenderPipelineDescription& desc = {}, const std::vector<WGPUConstantEntry>& constants = {});
    void create_render_async(Shader* shader, const WGPUColorTargetState& p_color_target, const RenderPipelineDescription& desc = {}, const std::vector<WGPUConstantEntry>& constants = {});

	//void create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout);
	void create_compute(Shader* shader, const std::string& entry_point = "compute", const std::vector<WGPUConstantEntry>& constants = {});

    //void create_compute_async(Shader* shader, WGPUPipelineLayout pipeline_layout);
    void create_compute_async(Shader* shader, const std::string& entry_point = "compute", const std::vector<WGPUConstantEntry>& constants = {});

	void reload(Shader* shader);

    bool set(const WGPURenderPassEncoder& render_pass) const;
    bool set(const WGPUComputePassEncoder& compute_pass) const;

    bool is_render_pipeline() const;
    bool is_compute_pipeline() const;

    bool is_msaa_allowed() const;

    inline bool is_loaded() const {
        return loaded;
    }

    friend void render_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPURenderPipeline pipeline, struct WGPUStringView message, void* userdata1, void* userdata2);
    friend void compute_pipeline_creation_callback(WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline, struct WGPUStringView message, void* userdata1, void* userdata2);

private:

    void create_render_common(Shader* shader, const WGPUColorTargetState& p_color_target, const RenderPipelineDescription& desc = {});
    void create_compute_common(Shader* shader);

    WGPUPipeline pipeline;

	WGPUColorTargetState    color_target;
	WGPUBlendState const*   blend_state = nullptr;
    RenderPipelineDescription     description;

    bool async_compile = false;
	bool loaded = false;
};
