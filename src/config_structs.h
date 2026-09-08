#pragma once

#include "framework/camera/camera.h"

#include <webgpu/webgpu.h>
#include <glm/vec3.hpp>

#include <vector>

struct sRendererConfig {

    WGPULimits required_limits = {};
    std::vector<WGPUFeatureName> required_features;

    uint8_t msaa_count = 1;

    sRendererConfig()
    {
        required_limits.maxVertexAttributes = 4;
        required_limits.maxVertexBuffers = 1;
        required_limits.maxBindGroups = 4;
        required_limits.maxUniformBuffersPerShaderStage = 1;
        required_limits.maxUniformBufferBindingSize = 65536;
        required_limits.minUniformBufferOffsetAlignment = 256;
        required_limits.minStorageBufferOffsetAlignment = 256;
        required_limits.maxComputeInvocationsPerWorkgroup = 256;
        required_limits.maxSamplersPerShaderStage = 1;
        required_limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

        required_features.push_back(WGPUFeatureName_TimestampQuery);
    }
};

typedef void (*EnginePostInitializeFunc)(void);
typedef void (*EngineUpdateFunc)(float);
typedef void (*EngineRenderFunc)(void);

struct sEngineConfig {
    uint16_t window_width = 1600;
    uint16_t window_height = 900;
    std::string window_title = "wgpuEngine APP";
    eCameraType camera_type = CAMERA_EDITOR;
    glm::vec3 camera_eye = { 0.0f, 0.75f, 2.0f };
    glm::vec3 camera_center = { 0.0f, 0.75f, 0.0f };
    bool fullscreen = false;

    sRendererConfig render_config = {};

    EnginePostInitializeFunc engine_post_initialize = nullptr;
    EngineUpdateFunc engine_pre_update = nullptr; // Updated before main scene
    EngineUpdateFunc engine_post_update = nullptr; // Updated after main scene
    EngineRenderFunc engine_render = nullptr;
};
