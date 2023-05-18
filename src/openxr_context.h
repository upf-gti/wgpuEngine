#pragma once

#include <vector>
#include <array>

#include "vulkan/vulkan.h"

#define USE_MIRROR_WINDOW

#define XR_USE_GRAPHICS_API_VULKAN
#include "openxr/openxr_platform.h"

struct OpenXRContext {

    XrInstance              xr_instance = XR_NULL_HANDLE;
    XrSystemId              xr_system_id;
    XrSession               xr_session;
    XrSwapchain             xr_swapchain;
    uint32_t                xr_view_count;
    XrFrameState            xr_frame_state {XR_TYPE_FRAME_STATE};

    std::vector<uint32_t>   last_acquired;

    std::vector<XrView> m_views;
    std::vector<XrViewConfigurationView> viewconfig_views;

    XrGraphicsBindingVulkan2KHR graphics_binding_gl;

    // small helper so we don't forget whether we treat 0 as left or right hand
    enum OPENXR_HANDS
    {
        HAND_LEFT = 0,
        HAND_RIGHT = 1,
        HAND_COUNT
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

    // Play space is usually local (head is origin, seated) or stage (room scale)
    XrSpace play_space;

    sInputState input_state;

    std::vector<XrCompositionLayerProjectionView>	projection_views;

    bool initialized = false;

    int initialize();
    void clean();
    bool xr_result(XrInstance xrInstance, XrResult result, const char* format, ...);
    void print_viewconfig_view_info();
    bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);
    void print_reference_spaces();
    void init_actions();

    void initFrame();
    void endFrame();
};
