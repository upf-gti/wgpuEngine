#pragma once

#include "core/managers/render/render_methods/render_method.h"

#include "core/error/error.h"

#include "framework/math/frustum_cull.h"
#include "scene/3d/camera_3d.h"

#include "glm/mat4x4.hpp"
#include "webgpu/webgpu.h"

#include <vector>

class Surface;
class Mesh;
class Material;
class RenderMethod;

struct sRenderableData {
    Mesh* mesh;
    glm::mat4x4 global_matrix;
};

class RenderCull {
    friend class RenderCompositor;

private:
    RenderCull() = default;
    ~RenderCull() = default;

    Error initialize();
    Error finalize();

    void set_render_method(RenderMethod* method);
    RenderMethod* get_render_method();

    void resize_swapchain();

    void set_frustum_camera_paused(bool value);
    bool get_frustum_camera_paused();

    void render(const std::vector<sRenderableData>& renderables_list, WGPUTextureView screen_surface_texture_view, const sEnvironmentData& environment_data);

    void prepare_cull_instancing(const Camera3D* camera, const std::vector<sRenderableData>& renderables_list, std::vector<std::vector<sRenderData>>& render_lists);

    Frustum frustum_cull;

    RenderMethod* render_method = nullptr;

    bool frustum_camera_paused = false;
};
