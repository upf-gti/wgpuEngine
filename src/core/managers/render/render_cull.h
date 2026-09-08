#pragma once

#include "core/error/error.h"
#include "core/managers/render/render_methods/render_method.h"

#include "framework/math/frustum_cull.h"
#include "scene/3d/camera_3d.h"

#include "glm/mat4x4.hpp"
#include "webgpu/webgpu.h"

#include <vector>

class Surface;
class Mesh;
class Material;

struct sRenderableData {
    Mesh* mesh;
    glm::mat4x4 global_matrix;
};

enum eRenderMethod {
    FORWARD_RENDERER,
    DEFERRED_RENDERER,
    CUSTOM_RENDERER
};

class RenderCull {
public:
    RenderCull() = default;
    ~RenderCull() = default;

    Error initialize();
    Error finalize();

    void set_render_method(RenderMethod* method);

    void resize_swapchain();

    void set_frustum_camera_paused(bool value);
    bool get_frustum_camera_paused();

    void render(const std::vector<sRenderableData>& renderables_list, const sEnvironmentData& environment_data);

private:
    void prepare_cull_instancing(const Camera3D* camera, const std::vector<sRenderableData>& renderables_list, std::vector<std::vector<sRenderData>>& render_lists);

    Frustum frustum_cull;

    RenderMethod* render_method = nullptr;

    bool frustum_camera_paused = false;
};
