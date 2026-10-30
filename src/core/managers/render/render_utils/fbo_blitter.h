#pragma once

#include "core/error/error.h"

#include "core/managers/render/render_api.h"

#include "graphics/pipeline.h"
#include "graphics/shader.h"
#include "graphics/surface.h"

class Surface;

class FBOBlitter {
public:
    FBOBlitter() = default;
    ~FBOBlitter() = default;

    Error initialize(WGPUTextureView source_texture_view, WGPUTextureFormat target_fbo_format, WGPUFilterMode filter_mode = WGPUFilterMode_Linear, bool apply_gamma = false);
    Error finalize();

    void blit(WGPUTextureView target_fbo_view);

    void set_source_texture(WGPUTextureView source_texture_view);
    void set_filter_mode(WGPUFilterMode filter_mode);

private:
    Shader* blitter_shader;
    Pipeline blitter_pipeline;
    WGPUBindGroup blit_bind_group;
    Surface quad_surface;
    WGPUTextureView source_texture_view = nullptr;

    WGPUFilterMode filter_mode = WGPUFilterMode_Linear;

    std::vector<Uniform*> blitter_uniforms;
};
