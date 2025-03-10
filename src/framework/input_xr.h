#pragma once

#include "includes.h"
#ifdef XR_SUPPORT
#include "xr/dawnxr/dawnxr.h"
#endif

#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"

#include <array>
#include <string>

// Small helper so we don't forget whether we treat 0 as left or right hand
enum OPENXR_HANDS
{
    HAND_LEFT = 0,
    HAND_RIGHT = 1,
    HAND_COUNT
};

enum OPENXR_POSES {
    POSE_GRIP = 0,
    POSE_AIM
};

enum OPENXR_BUTTONS {
    XR_BUTTON_A = 0,
    XR_BUTTON_B,
    XR_BUTTON_X,
    XR_BUTTON_Y,
    XR_BUTTON_MENU,
};

enum OPENXR_THUMBSTICK_AXIS : uint8_t {
    XR_THUMBSTICK_NO_AXIS = 0,
    XR_THUMBSTICK_AXIS_X,
    XR_THUMBSTICK_AXIS_Y
};

#define XR_THUMBSTICK_DEADZONE 0.01f

#ifdef XR_SUPPORT

struct XrActionStorage {
    bool active = false;
    XrPath path;
    XrAction action;
    XrActionStateBoolean state;
};

struct XrMappedButtonState {
    std::string name;
    uint8_t hand;
    XrActionStorage click;
    XrActionStorage touch;

    void bind_click(XrInstance* instance, const char* path) { click.active = true; xrStringToPath(*instance, path, &click.path); }
    void bind_touch(XrInstance* instance, const char* path) { touch.active = true; xrStringToPath(*instance, path, &touch.path); }
};

struct XrInputPose {
    glm::quat orientation;
    glm::vec3 position;
};

struct sSwapchainData {
    XrSwapchain swapchain = {};
    uint32_t    image_index = {};
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

struct XrInputData {

    // Poses
    glm::mat4x4 eyePoseMatrixes[EYE_COUNT];
    XrInputPose eyePoses[EYE_COUNT];
    glm::mat4x4 headPoseMatrix;
    XrInputPose headPose;
    glm::mat4x4 controllerAimPoseMatrices[HAND_COUNT];
    XrInputPose controllerAimPoses[HAND_COUNT];
    glm::mat4x4 controllerGripPoseMatrices[HAND_COUNT];
    XrInputPose controllerGripPoses[HAND_COUNT];

    // Input States. Also includes lastChangeTime, isActive, changedSinceLastSync properties.
    XrActionStateFloat grabState[HAND_COUNT];
    XrActionStateVector2f thumbStickValueState[HAND_COUNT];
    XrActionStateBoolean thumbStickClickState[HAND_COUNT];
    XrActionStateBoolean thumbStickTouchState[HAND_COUNT];
    XrActionStateFloat triggerValueState[HAND_COUNT];
    XrActionStateBoolean triggerTouchState[HAND_COUNT];

    // Buttons.
    std::vector<XrMappedButtonState> buttonsState;

    // Headset State. Use to detect status / user proximity / user presence / user engagement https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#session-lifecycle
    XrSessionState headsetActivityState = XR_SESSION_STATE_UNKNOWN;
};

#endif
