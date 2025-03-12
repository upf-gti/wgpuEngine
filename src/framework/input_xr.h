#pragma once

#include "includes.h"
#ifdef XR_SUPPORT
#include "xr/dawnxr/dawnxr.h"
#endif

#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"

#include <array>
#include <string>

#ifdef OPENXR_SUPPORT

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

#endif
