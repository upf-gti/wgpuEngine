#pragma once

#include <vector>
#include <array>
#include "includes.h"

#ifdef XR_SUPPORT

#include "dawnxr/dawnxr.h"
#include "framework/input_xr.h"

#if defined(BACKEND_VULKAN)
struct XrGraphicsRequirementsVulkanKHR;
#elif defined(BACKEND_DX12)
struct XrGraphicsRequirementsD3D12KHR;
#endif

struct WebGPUContext;

struct OpenXRContext {

    /*
    * XR General
    */

    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId system_id;
    // Play space is usually local (head is origin, seated) or stage (room scale)
    XrSpace play_space;
    bool initialized = false;

    bool init(WebGPUContext* webgpu_context);
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

    float z_near = 0.01f;
    float z_far = 1000.0f;

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

    void update();

    /*
    * Debug & Errors
    */

    void print_viewconfig_view_info();

    int check_backend_requirements();

#if defined(BACKEND_VULKAN)
    bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);
#elif defined(BACKEND_DX12)
    bool check_dx12_version(XrGraphicsRequirementsD3D12KHR* dx12_reqs);
#endif

    void print_reference_spaces();

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
