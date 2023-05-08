#pragma once

#include <webgpu.h>
#include "utils.h"
#include "wgpu.h"

#include <GLFW/glfw3.h>
#include <array>
#include <vector>

#include "vulkan/vulkan.h"

#define XR_USE_GRAPHICS_API_VULKAN
#include "openxr/openxr_platform.h"

//https://eliemichel.github.io/LearnWebGPU/getting-started/hello-webgpu.html
// Add this in order to remain compatible with Dawn
#define wgpuInstanceRelease wgpuInstanceDrop

// small helper so we don't forget whether we treat 0 as left or right hand
enum OPENXR_HANDS
{
    HAND_LEFT = 0,
    HAND_RIGHT = 1,
    HAND_COUNT
};

namespace WGPUEnv {

    // hand tracking extension data
    struct
    {
        bool supported;
        // whether the current VR system in use has hand tracking
        bool system_supported;
        PFN_xrLocateHandJointsEXT pfnLocateHandJointsEXT;
        std::array<XrHandTrackerEXT, HAND_COUNT> trackers;
    } hand_tracking;

    //// depth layer data
    //struct
    //{
    //    bool supported;
    //    std::vector<XrCompositionLayerDepthInfoKHR> infos;
    //} depth;

    //// cylinder layer extension data
    //struct
    //{
    //    bool supported;
    //    int64_t format;
    //    uint32_t swapchain_width, swapchain_height;
    //    uint32_t swapchain_length;
    //    std::vector<XrSwapchainImageVulkanKHR> images;
    //    XrSwapchain swapchain;
    //} cylinder;

    struct sInstance {
        WGPUInstance            wgpuInstance;
        WGPUAdapter             adapter;
        WGPUSurface             surface;
        WGPUDevice              device;
        WGPUQueue               device_queue;
        WGPUCommandEncoder      device_command_encoder;
        WGPUTextureFormat       swapchain_format;
        WGPUSwapChain           swapchain;
        WGPUShaderModule        shader_module;
        WGPURenderPipeline      render_pipeline;
        WGPUPipelineLayout      render_pipeline_layout;

        bool                    is_initialized = false;
        WGPUTextureView         current_texture_view;

        // OpenXR ==========================
        XrInstance              openXRInstance = XR_NULL_HANDLE;
        XrSystemId              systemId;
        int initialize_openxr();
        bool xr_result(XrInstance wgpuInstance, XrResult result, const char* format, ...);
        bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);

        // Vulkan ==========================
        VkInstance              vulkanInstance;
        std::vector<const char*> getRequiredExtensions();

        // Methods =========================
        void initialize(GLFWwindow *window, void* callback);
        void clean();

        void render_frame();

        void _config_with_device();

        void _config_render_pipeline();

        // Events =========================
        static void e_device_error(WGPUErrorType type, char const* message, void* user_data);
        static void e_adapter_request_ended(WGPURequestAdapterStatus status, 
                                            WGPUAdapter adapter, 
                                            char const* message, 
                                            void* user_data);
        static void e_device_request_ended(WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void *user_data);
    };


    struct sWGPUInitializationPayload {
        sInstance  *wgpu_man_instance = NULL; 
        GLFWwindow *window = NULL;
        void *callback = NULL;
    };
};