#pragma once

#include "core/error/error.h"
#include "core/managers/render/render_cull.h"
#include "core/managers/render/render_methods/render_method.h"

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

    void resize_swapchain();

    RenderCull render_cull;
};
