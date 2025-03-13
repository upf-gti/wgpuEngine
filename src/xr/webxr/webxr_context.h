#pragma once

#include "xr/xr_context.h"

#include "includes.h"

// #define WEBXR_SUPPORT

#ifdef WEBXR_SUPPORT

#include "webxr.h"

// WEBXR_BUTTON_THUMBSTICK,
enum WEBXR_BUTTONS
{
    WEBXR_BUTTON_TRIGGER = 0,
    WEBXR_BUTTON_GRIP,
    WEBXR_BUTTON_TOUCHPAD,
    WEBXR_BUTTON_THUMBSTICK_PRESS,
    WEBXR_BUTTON_AX,
    WEBXR_BUTTON_BY,
    WEBXR_BUTTON_COUNT
};

struct WebXRContext : public XRContext {

    virtual ~WebXRContext();

    /*
    * XR General
    */

    bool initialized = false;

    bool init(WebGPUContext* webgpu_context) override;
    void clean() override;

    /*
    * XR Input
    */

    struct XrInputData {

        // Poses
        // glm::mat4x4 eyePoseMatrixes[EYE_COUNT];
        // XrInputPose eyePoses[EYE_COUNT];
        glm::mat4x4 headPoseMatrix;
        WebXRRigidTransform headPose;
        glm::mat4x4 controllerAimPoseMatrices[HAND_COUNT];
        WebXRRigidTransform controllerAimPoses[HAND_COUNT];
        glm::mat4x4 controllerGripPoseMatrices[HAND_COUNT];
        WebXRRigidTransform controllerGripPoses[HAND_COUNT];

        std::vector<float> axisState[HAND_COUNT];

        // Buttons.
        std::vector<GamepadButton> buttonsState[HAND_COUNT];

        // // Headset State. Use to detect status / user proximity / user presence / user engagement https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#session-lifecycle
        // XrSessionState headsetActivityState = XR_SESSION_STATE_UNKNOWN;
    };

    XrInputData xr_data;

    //sInputState input_state;

    //void init_actions(XrInputData& data);
    //void poll_actions(XrInputData& data);

    //void apply_haptics(uint8_t controller, float amplitude, float duration);
    //void stop_haptics(uint8_t controller);

    /*
    * XR Session
    */

    bool begin_session() override;
    bool end_session() override;

    /*
    * Render
    */

    void update_views(WebXRRigidTransform* head_pose, WebXRView views[2], WGPUTextureView texture_view_left, WGPUTextureView texture_view_right);

    WGPUTextureView swapchain_views[2];

    WGPUTextureView get_swapchain_view(uint8_t eye_idx) override;

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

    void update() override;

    /*
    * Debug & Errors
    */

    void print_viewconfig_view_info() override;

    void print_reference_spaces() override;

    void print_error(int error);

private:

};

#else

struct WebXRContext {

};

#endif
