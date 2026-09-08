#pragma once

#include "core/error/error.h"

#include "graphics/uniform.h"

#include "webgpu/webgpu.h"

#include <vector>

class Surface;
class Mesh;
class Material;

struct sRenderData {
    Surface* surface;
    uint32_t repeat;
    glm::mat4x4 global_matrix;
    Mesh* mesh_ref;
    Material* material;
};

struct sInstanceData {
    std::vector<sUniformData> instances_data[RENDER_LIST_COUNT];
    Uniform instances_data_uniforms[RENDER_LIST_COUNT];
    WGPUBindGroup instances_bind_groups[RENDER_LIST_COUNT] = {};
};

class RenderMethod {
    friend class RenderManager;

public:
    virtual void render_scene(const std::vector<std::vector<sRenderData>>& render_lists) = 0;

    virtual void resize_swapchain() = 0;

private:
    virtual Error initialize() = 0;
    virtual Error finalize() = 0;

protected:
    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
};
