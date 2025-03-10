#pragma once

#include "xr_context.h"

#ifdef XR_SUPPORT

#include "dawnxr/dawnxr.h"
#include "framework/input_xr.h"

#if defined(BACKEND_VULKAN)
struct XrGraphicsRequirementsVulkanKHR;
#elif defined(BACKEND_DX12)
struct XrGraphicsRequirementsD3D12KHR;
#endif

struct OpenXRContext : public XRContext {

    /*
    * XR General
    */

    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId system_id;
    // Play space is usually local (head is origin, seated) or stage (room scale)
    XrSpace play_space;

    bool init(WebGPUContext* webgpu_context) override;
    void clean() override;

    bool create_instance();
    XrInstance* get_instance() { return &instance; }

    /*
    * XR Input
    */

    sInputState input_state;

    void init_actions(XrInputData& data);
    void poll_actions(XrInputData& data);

    void apply_haptics(uint8_t controller, float amplitude, float duration);
    void stop_haptics(uint8_t controller);

    /*
    * XR Session
    */

    XrSession session;
    XrSessionState session_state;

    bool begin_session() override;
    bool end_session() override;

    /*
    * Render
    */

    WGPUTextureView get_swapchain_view(uint8_t eye_idx) override;
    uint32_t get_swapchain_image_index(uint8_t eye_idx) override;

    uint32_t view_count;
    XrFrameState frame_state{ XR_TYPE_FRAME_STATE };
    uint32_t swapchain_length; // Number of textures per swapchain

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

    std::vector<sSwapchainData> swapchains;
    std::vector<XrView> views;
    std::vector<XrViewConfigurationView> viewconfig_views;
    std::vector<XrCompositionLayerProjectionView> projection_views;
    std::vector<int64_t> swapchain_formats;

    void init_frame();
    void acquire_swapchain(int swapchain_index);
    void release_swapchain(int swapchain_index);
    void end_frame();

    void update();

    /*
    * Debug & Errors
    */

    void print_viewconfig_view_info() override;
    void print_reference_spaces() override;

    int check_backend_requirements();

#if defined(BACKEND_VULKAN)
    bool check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs);
#elif defined(BACKEND_DX12)
    bool check_dx12_version(XrGraphicsRequirementsD3D12KHR* dx12_reqs);
#endif

private:
    
    bool create_action(XrActionSet actionSet,
        XrPath* paths,
        uint32_t num_paths,
        const std::string& actionName,
        const std::string& localizedActionName,
        XrActionType type,
        XrAction& action);

    XrActionStatePose get_action_pose_state(XrAction targetAction, uint8_t controller);
    XrActionStateBoolean get_action_boolean_state(XrAction targetAction, uint8_t controller);
    XrActionStateFloat get_action_float_state(XrAction targetAction, uint8_t controller);
    XrActionStateVector2f get_action_vector2f_State(XrAction targetAction, uint8_t controller);
};

#else

struct XRContext {

};

#endif
