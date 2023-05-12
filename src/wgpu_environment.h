#pragma once

#include <dawnxr/dawnxr.h>
#include "utils.h"

#include <GLFW/glfw3.h>
#include <array>
#include <vector>

#include "vulkan/vulkan.h"

#include <dawn/native/DawnNative.h>

#define XR_USE_GRAPHICS_API_VULKAN
#include "openxr/openxr_platform.h"

namespace dawnxr::internal {
    struct Session;
}

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
        dawn::native::Instance*   dawnInstance;
        //wgpu::Instance            wgpuInstance;
        //wgpu::Adapter             adapter;
        //wgpu::Surface             surface;
        wgpu::Device              device;
        wgpu::Queue               device_queue;
        wgpu::CommandEncoder      device_command_encoder;
        wgpu::TextureFormat       swapchain_format;
        //wgpu::SwapChain           swapchain;
        wgpu::ShaderModule        shader_module;
        wgpu::RenderPipeline      render_pipeline;
        wgpu::PipelineLayout      render_pipeline_layout;

        bool                      is_initialized = false;
        wgpu::TextureView         current_texture_view;

        dawn::native::AdapterDiscoveryOptionsBase** options;

        // OpenXR ==========================
        XrInstance              xr_instance = XR_NULL_HANDLE;
        XrSystemId              xr_system_id;
        XrSession               xr_session;
        XrSwapchain             xr_swapchain;
        uint32_t                xr_view_count;
        XrFrameState            xr_frame_state;

        std::vector<uint32_t>   last_acquired;

        std::vector<dawnxr::SwapchainImageDawn> images;

        std::vector<XrViewConfigurationView> viewconfig_views;

        XrGraphicsBindingVulkan2KHR graphics_binding_gl;

        // Play space is usually local (head is origin, seated) or stage (room scale)
        XrSpace			        play_space;

        std::vector<XrCompositionLayerProjectionView>	projection_views;

        int initialize_openxr();
        bool xr_result(XrInstance wgpuInstance, XrResult result, const char* format, ...);
        bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);
        void print_viewconfig_view_info();
        void print_reference_spaces();

        // Methods =========================
        int initialize(GLFWwindow *window, void* callback);
        void clean();

        void render_frame();

        void _config_with_device();

        void _config_render_pipeline();

        // Events =========================
        static void e_device_error(WGPUErrorType type, char const* message, void* user_data);
    };


    struct sWGPUInitializationPayload {
        sInstance  *wgpu_man_instance = NULL; 
        GLFWwindow *window = NULL;
        void *callback = NULL;
    };
};