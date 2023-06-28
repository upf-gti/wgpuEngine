#pragma once

#include <vector>
#include <array>
#include "includes.h"
#include "framework/input.h"

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

enum OPENXR_EYES
{
    EYE_LEFT = 0,
    EYE_RIGHT = 1,
    EYE_COUNT
};

struct sPoseData {
    glm::quat orientation;
    glm::vec3 position;
};

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
    bool system_supported;
    PFN_xrLocateHandJointsEXT pfnLocateHandJointsEXT;
    std::array<XrHandTrackerEXT, HAND_COUNT> trackers;
} hand_tracking;

struct sInputState {
    XrActionSet actionSet{ XR_NULL_HANDLE };
    XrAction grabAction{ XR_NULL_HANDLE };
    XrAction poseAction{ XR_NULL_HANDLE };
    XrAction vibrateAction{ XR_NULL_HANDLE };
    XrAction thumbstickAction{ XR_NULL_HANDLE };
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

    int  initialize(WebGPUContext* webgpu_context);
    void clean();
    bool create_instance();
    bool xr_result(XrInstance xrInstance, XrResult result, const char* format, ...);
    void print_viewconfig_view_info();
    bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);
    void print_reference_spaces();
    
    // XR Input
    void init_actions();
    void sync(XrInputData& input_data);

    void init_frame();
    void acquire_swapchain(int swapchain_index);
    void release_swapchain(int swapchain_index);
    void end_frame();

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