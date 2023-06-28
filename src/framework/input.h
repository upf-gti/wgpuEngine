#pragma once

#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include <vector>

class Renderer;

XrAction grabAction{ XR_NULL_HANDLE };
XrAction thumbstickAction{ XR_NULL_HANDLE };
XrAction poseAction{ XR_NULL_HANDLE };
XrAction vibrateAction{ XR_NULL_HANDLE };
XrAction quitAction{ XR_NULL_HANDLE };

struct XrInputData {
    
    // [tdbe] Poses
    glm::mat4 eyePoseMatrixes[EYE_COUNT];
    sPoseData eyePoses[EYE_COUNT];
    glm::mat4 headPoseMatrix;
    sPoseData headPose;
    glm::mat4 controllerAimPoseMatrixes[HAND_COUNT];
    sPoseData controllerAimPoses[HAND_COUNT];
    glm::mat4 controllerGripPoseMatrixes[HAND_COUNT];
    sPoseData controllerGripPoses[HAND_COUNT];

    // [tdbe] Input States. Also includes lastChangeTime, isActive, changedSinceLastSync properties.
    XrActionStateFloat grabState[HAND_COUNT] = {XR_TYPE_ACTION_STATE_FLOAT};
    XrActionStateVector2f thumbStickState[HAND_COUNT] = {XR_TYPE_ACTION_STATE_VECTOR2F};
    XrActionStateBoolean quitClickState[HAND_COUNT] = {XR_TYPE_ACTION_STATE_BOOLEAN};
    /*XrActionStateBoolean selectClickState[HAND_COUNT] = {XR_TYPE_ACTION_STATE_BOOLEAN};
    XrActionStateFloat triggerState[HAND_COUNT] = {XR_TYPE_ACTION_STATE_BOOLEAN};*/


    // [tdbe] Headset State. Use to detect status / user proximity / user presence / user engagement https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#session-lifecycle
    XrSessionState headsetActivityState = XR_SESSION_STATE_UNKNOWN;
};

class Input {

	// Mouse state
	static glm::vec2 mouse_position; //last mouse position
	static glm::vec2 mouse_delta; //mouse movement in the last frame
	static float mouse_wheel;
	static float mouse_wheel_delta;
	static uint8_t buttons[GLFW_MOUSE_BUTTON_LAST];
	static uint8_t prev_buttons[GLFW_MOUSE_BUTTON_LAST];

	// Keyboard
	static uint8_t keystate[GLFW_KEY_LAST];
	static uint8_t prev_keystate[GLFW_KEY_LAST];

	static GLFWwindow* window;

	static bool use_mirror_screen;

    static XrInputData xr_data;

public:

	static void init(GLFWwindow* window, Renderer* renderer);
	static void update(float delta_time);
	static void center_mouse();

	// https://www.glfw.org/docs/3.3/group__keys.html
	static bool is_key_pressed(uint8_t key) { return keystate[key] == GLFW_PRESS; }
	static bool was_key_pressed(uint8_t key) { return prev_keystate[key] == GLFW_RELEASE && keystate[key] == GLFW_PRESS; }

	// https://www.glfw.org/docs/3.3/group__buttons.html
	static bool is_mouse_pressed(uint8_t button) { return buttons[button] == GLFW_PRESS; }
	static bool was_mouse_pressed(uint8_t button) { return prev_buttons[button] == GLFW_RELEASE && buttons[button] == GLFW_PRESS; }
};