#pragma once

#include "core/error/error.h"
#include "core/managers/render/render_cull.h"
#include "core/managers/render/render_methods/render_method.h"
#include "core/managers/render/render_utils/fbo_blitter.h"

#include "framework/math/frustum_cull.h"
#include "scene/3d/camera_3d.h"

#include "glm/mat4x4.hpp"
#include "webgpu/webgpu.h"

#include <vector>

class RenderCompositor {
    friend class RenderManager;

private:
    RenderCompositor() = default;
    ~RenderCompositor() = default;

    Error initialize();
    Error finalize();

    void set_render_method(RenderMethod* method);
    RenderMethod* get_render_method();

    void render(const std::vector<sRenderableData>& renderables_list, const sEnvironmentData& environment_data);

    void set_render_size(int32_t width, int32_t height);

    void render_imgui(WGPUTextureView screen_surface_texture_view);

    void init_render_fbo(int32_t width, int32_t height);

    RenderCull render_cull;

    WGPUTexture render_fbo = nullptr;
    WGPUTextureView render_fbo_view = nullptr;

    FBOBlitter blitter;
};
