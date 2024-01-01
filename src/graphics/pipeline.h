#pragma once

#include <webgpu/webgpu.h>

#include "webgpu_context.h"

#include <variant>
#include <unordered_map>

#include "utils.h"

class Shader;
class Mesh;

struct PipelineDescription {

    WGPUCullMode cull_mode = WGPUCullMode_Back;
    WGPUPrimitiveTopology topology = WGPUPrimitiveTopology_TriangleList;

    bool uses_depth_buffer  = true;
    bool uses_depth_write   = true;
    bool blending_enabled   = false;
};

struct RenderPipelineKey {
    Shader* shader;
    WGPUColorTargetState color_target;
    PipelineDescription description;

    bool operator==(const RenderPipelineKey& other) const
    {
        return (shader == other.shader
            && color_target.format == other.color_target.format
            && color_target.writeMask == other.color_target.writeMask
            && color_target.blend == other.color_target.blend
            && description.cull_mode == other.description.cull_mode
            && description.topology == other.description.topology);
    }
};

template <>
struct std::hash<RenderPipelineKey>
{
    std::size_t operator()(const RenderPipelineKey& k) const
    {
        using std::size_t;
        using std::hash;
        using std::string;

        std::size_t h1 = hash<void*>()(k.shader);
        std::size_t h2 = hash<uint32_t>()(static_cast<uint32_t>(k.color_target.format));
        std::size_t h3 = hash<uint32_t>()(static_cast<uint32_t>(k.color_target.writeMask));
        std::size_t h4 = hash<void const*>()(k.color_target.blend);
        std::size_t h5 = hash<uint32_t>()(static_cast<uint32_t>(k.description.cull_mode));
        std::size_t h6 = hash<uint32_t>()(static_cast<uint32_t>(k.description.topology));

        std::size_t seed = 0;
        hash_combine(seed, h1, h2, h3, h4, h5, h6);
        return seed;
    }
};

class Pipeline {

public:

	~Pipeline();

	static WebGPUContext* webgpu_context;

    void create_render(Shader* shader, WGPUColorTargetState p_color_target, PipelineDescription desc = {});

	void create_compute(Shader* shader, WGPUPipelineLayout pipeline_layout);
	void create_compute(Shader* shader);

    static void register_render_pipeline(Shader* shader, WGPUColorTargetState p_color_target, PipelineDescription desc = {});
    static void register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout);
    static void clean_registered_pipelines();

	void reload(Shader* shader);

	void set(const WGPURenderPassEncoder& render_pass);
	void set(const WGPUComputePassEncoder& compute_pass);

private:

	WGPUPipelineLayout pipeline_layout = nullptr;
	std::variant<std::monostate, WGPURenderPipeline, WGPUComputePipeline> pipeline;

    static std::unordered_map<RenderPipelineKey, Pipeline*> registered_render_pipelines;
    static std::unordered_map<Shader*, Pipeline*> registered_compute_pipelines;

	WGPUColorTargetState    color_target;
	WGPUBlendState const*   blend_state = nullptr;
    PipelineDescription     description;

	bool loaded = false;
};
