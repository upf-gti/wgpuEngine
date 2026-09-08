#pragma once

#include "core/error/error.h"
#include "core/managers/render/render_methods/render_method.h"

#include "framework/camera/camera.h"
#include "framework/math/frustum_cull.h"

#include "glm/mat4x4.hpp"
#include "webgpu/webgpu.h"

#include <vector>

class Surface;
class Mesh;
class Material;

struct sUniformData {
    glm::mat4x4 model;
};

struct sRenderableData {
    Mesh* mesh;
    glm::mat4x4 global_matrix;
};

enum eRenderListType {
    RENDER_LIST_OPAQUE,
    RENDER_LIST_TRANSPARENT,
    RENDER_LIST_SPLATS,
    RENDER_LIST_2D,
    RENDER_LIST_2D_TRANSPARENT,
    RENDER_LIST_COUNT
};

class RenderCull {
public:
    Error initialize();
    Error finalize();

    void set_render_method(RenderMethod* method);

    void resize_swapchain();

    void set_frustum_camera_paused(bool value);
    bool get_frustum_camera_paused();

    void render(const std::vector<sRenderableData>& renderables_list);

private:
    RenderCull() = default;
    ~RenderCull() = default;

    void prepare_cull_instancing(const Camera* camera, const std::vector<sRenderableData>& renderables_list, std::vector<std::vector<sRenderData>>& render_lists);

    Frustum frustum_cull;

    RenderMethod* render_method = nullptr;

    bool frustum_camera_paused = false;
};
