#pragma once

#include <vector>
#include <array>
#include "includes.h"

#ifdef XR_SUPPORT

#include "vulkan/vulkan.h"

#define XR_USE_GRAPHICS_API_VULKAN
#include "openxr/openxr_platform.h"

#include <dawnxr/dawnxr.h>

#include "graphics/webgpu_context.h"

// small helper so we don't forget whether we treat 0 as left or right hand
enum OPENXR_HANDS
{
    HAND_LEFT = 0,
    HAND_RIGHT = 1,
    HAND_COUNT
};

struct sViewData {
    glm::mat4x4 projection_matrix;
    glm::mat4x4 view_matrix;
};

struct sSwapchainData {
    XrSwapchain swapchain;
    uint32_t    image_index;
    std::vector<dawnxr::SwapchainImageDawn> images;
};

struct
{
    bool supported = false;
    // whether the current VR system in use has hand tracking
    bool system_supported;
    PFN_xrLocateHandJointsEXT pfnLocateHandJointsEXT;
    std::array<XrHandTrackerEXT, HAND_COUNT> trackers;
} hand_tracking;

struct sInputState {
    XrActionSet actionSet{ XR_NULL_HANDLE };
    XrAction grabAction{ XR_NULL_HANDLE };
    XrAction poseAction{ XR_NULL_HANDLE };
    XrAction vibrateAction{ XR_NULL_HANDLE };
    XrAction quitAction{ XR_NULL_HANDLE };

    XrPath handSubactionPath[HAND_COUNT];
    XrSpace handSpace[HAND_COUNT];
    float handScale[HAND_COUNT] = { 1.0f, 1.0f };
    XrBool32 handActive[HAND_COUNT];
};

struct OpenXRContext {

    XrInstance                  instance = XR_NULL_HANDLE;
    XrSystemId                  system_id;
    XrSession                   session;
    uint32_t                    view_count;
    XrFrameState                frame_state {XR_TYPE_FRAME_STATE};
    std::vector<sSwapchainData> swapchains;
    uint32_t                    swapchain_length; // Number of textures per swapchain

    uint32_t                    extension_count = 0;

    std::vector<XrView>                             views;
    std::vector<XrViewConfigurationView>            viewconfig_views;
    std::vector<XrCompositionLayerProjectionView>	projection_views;

    std::vector<sViewData>                          per_view_data;

    std::vector<int64_t>                            swapchain_formats;

    XrGraphicsBindingVulkan2KHR graphics_binding_gl;

    // Play space is usually local (head is origin, seated) or stage (room scale)
    XrSpace play_space;

    sInputState input_state;

    bool initialized = false;

    int initialize(WebGPUContext* webgpu_context);
    void clean();
    bool isOpenXRAvailable();
    int createInstance();
    bool xr_result(XrInstance xrInstance, XrResult result, const char* format, ...);
    void print_viewconfig_view_info();
    bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);
    void print_reference_spaces();
    void init_actions();

    void initFrame();
    void acquireSwapchain(int swapchain_index);
    void releaseSwapchain(int swapchain_index);
    void endFrame();
};

#endif