#pragma once

#include <vector>
#include <array>
#include "includes.h"
#include "framework/input.h"

#ifdef XR_SUPPORT

#include <dawnxr/dawnxr.h>
#include "openxr/openxr_platform.h"

#include "graphics/webgpu_context.h"

struct sViewData {
    glm::vec3   position;
    glm::mat4x4 projection_matrix;
    glm::mat4x4 view_matrix;
    glm::mat4x4 view_projection_matrix;
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
    bool system_supported = false;
    PFN_xrLocateHandJointsEXT pfnLocateHandJointsEXT = {};
    std::array<XrHandTrackerEXT, HAND_COUNT> trackers = {};
} hand_tracking;

struct sInputState {

    XrActionSet actionSet{ XR_NULL_HANDLE };
    
    // hand pose: point in the world using the input source, according to the platform's conventions for aiming with that kind of source.
    XrAction aimPoseAction{ XR_NULL_HANDLE };
    // hand pose: render a virtual object held in the user's hand, whether it is tracked directly or by a motion controller.
    XrAction gripPoseAction{ XR_NULL_HANDLE };

    XrAction grabAction{ XR_NULL_HANDLE };
    XrAction thumbstickValueAction{ XR_NULL_HANDLE };
    XrAction thumbstickClickAction{ XR_NULL_HANDLE };
    XrAction thumbstickTouchAction{ XR_NULL_HANDLE };
    XrAction triggerValueAction{ XR_NULL_HANDLE };
    XrAction triggerTouchAction{ XR_NULL_HANDLE };
    XrAction vibrateAction{ XR_NULL_HANDLE };

    // Buttons.
    // There are stored in data input...

    XrPath handSubactionPath[HAND_COUNT];
    XrSpace aimHandSpace[HAND_COUNT];
    XrSpace gripHandSpace[HAND_COUNT];
};

struct OpenXRContext {

    /*
    * XR General
    */

    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId system_id;
    // Play space is usually local (head is origin, seated) or stage (room scale)
    XrSpace play_space;
    bool initialized = false;

    int init(WebGPUContext* webgpu_context);
    void clean();
    bool create_instance();
    XrInstance* get_instance() { return &instance; }

    /*
    * XR Input
    */

    sInputState input_state;

    void init_actions(XrInputData& data);
    void poll_actions(XrInputData& data);

    /*
    * XR Session
    */

    XrSession session;
    XrSessionState session_state;

    bool begin_session();
    bool end_session();

    /*
    * Render
    */

    uint32_t view_count;
    XrFrameState frame_state{ XR_TYPE_FRAME_STATE };
    uint32_t swapchain_length; // Number of textures per swapchain
    XrGraphicsBindingVulkan2KHR graphics_binding_gl;

    std::vector<sSwapchainData> swapchains;
    std::vector<XrView> views;
    std::vector<XrViewConfigurationView> viewconfig_views;
    std::vector<XrCompositionLayerProjectionView> projection_views;
    std::vector<sViewData> per_view_data;
    std::vector<int64_t> swapchain_formats;

    void init_frame();
    void acquire_swapchain(int swapchain_index);
    void release_swapchain(int swapchain_index);
    void end_frame();

    /*
    * Debug & Errors
    */

    void print_viewconfig_view_info();
    bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);
    void print_reference_spaces();
    bool xr_result(XrInstance xrInstance, XrResult result, const char* format, ...);

private:
    
    bool create_action(XrActionSet actionSet,
        XrPath* paths,
        uint32_t num_paths,
        const std::string& actionName,
        const std::string& localizedActionName,
        XrActionType type,
        XrAction& action);

    XrActionStatePose get_action_pose_state(XrAction targetAction, int controller);
    XrActionStateBoolean get_action_boolean_state(XrAction targetAction, int controller);
    XrActionStateFloat get_action_float_state(XrAction targetAction, int controller);
    XrActionStateVector2f get_action_vector2f_State(XrAction targetAction, int controller);
};

#endif