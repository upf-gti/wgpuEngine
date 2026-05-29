#pragma once

#include "framework/camera/camera.h"

#include <webgpu/webgpu.h>
#include <glm/vec3.hpp>

#include <vector>

class Engine;
class Renderer;

struct sRendererConfiguration {
    WGPULimits required_limits = {};
    std::vector<WGPUFeatureName> features;

    sRendererConfiguration()
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

        features.push_back(WGPUFeatureName_TimestampQuery);
    }
};

typedef void (*EnginePostInitializeFunc)(void);
typedef void (*EngineUpdateFunc)(float);
typedef void (*EngineRenderFunc)(void);

struct sEngineConfiguration {
    uint16_t window_width = 1600;
    uint16_t window_height = 900;
    std::string window_title = "wgpuEngine";
    eCameraType camera_type = CAMERA_FLYOVER;
    glm::vec3 camera_eye = { 0.0f, 0.75f, 2.0f };
    glm::vec3 camera_center = { 0.0f, 0.75f, 0.0f };
    uint8_t msaa_count = 1;
    bool fullscreen = false;

    sRendererConfiguration render_config = {};

    EnginePostInitializeFunc engine_post_initialize = nullptr;
    EngineUpdateFunc engine_pre_update = nullptr; // Updated before main scene
    EngineUpdateFunc engine_post_update = nullptr; // Updated after main scene
    EngineRenderFunc engine_render = nullptr;

    // To allow deprecated inheritance behavior
    Engine* custom_engine_instance = nullptr;
    Renderer* custom_renderer_instance = nullptr;
};
