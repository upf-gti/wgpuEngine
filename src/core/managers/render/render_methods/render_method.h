#pragma once

#include "core/error/error.h"

#include "graphics/uniform.h"
#include "scene/3d/camera_3d.h"

#include "webgpu/webgpu.h"

#include <vector>

class Surface;
class Mesh;
class Material;
class Texture;

struct sRenderData {
    Surface* surface;
    uint32_t repeat;
    glm::mat4x4 global_matrix;
    Mesh* mesh_ref;
    Material* material;
};

enum eRenderListType {
    RENDER_LIST_OPAQUE,
    RENDER_LIST_TRANSPARENT,
    RENDER_LIST_SPLATS,
    RENDER_LIST_2D,
    RENDER_LIST_2D_TRANSPARENT,
    RENDER_LIST_COUNT
};

struct sUniformData {
    glm::mat4x4 model;
};

struct sInstanceData {
    std::vector<sUniformData> instances_data[RENDER_LIST_COUNT];
    Uniform instances_data_uniforms[RENDER_LIST_COUNT];
    WGPUBindGroup instances_bind_groups[RENDER_LIST_COUNT] = {};
};

struct sEnvironmentData {
    Texture* irradiance_texture_panorama;
};

class RenderMethod {
    friend class RenderManager;

public:
    virtual void render_scene(std::vector<std::vector<sRenderData>>& render_lists, const Camera3D* camera, const sEnvironmentData& environment_data, WGPUTextureView framebuffer_view, eEYE eye_idx = EYE_LEFT, const char* label = nullptr) = 0;

    virtual void resize_swapchain() = 0;

private:
    virtual Error initialize() = 0;
    virtual Error finalize() = 0;

protected:
    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
};
