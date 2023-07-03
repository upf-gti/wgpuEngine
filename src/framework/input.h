#pragma once

#include "includes.h"
#include <GLFW/glfw3.h>
#include <vector>

// Small helper so we don't forget whether we treat 0 as left or right hand
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

#ifdef XR_SUPPORT

#include "vulkan/vulkan.h"
#define XR_USE_GRAPHICS_API_VULKAN
#include "openxr/openxr_platform.h"

struct XrInputPose {
	glm::quat orientation;
	glm::vec3 position;
};

struct XrInputData {

	// [tdbe] Poses
	glm::mat4x4 eyePoseMatrixes[EYE_COUNT];
	XrInputPose eyePoses[EYE_COUNT];
	glm::mat4x4 headPoseMatrix;
	XrInputPose headPose;
	glm::mat4x4 controllerAimPoseMatrixes[HAND_COUNT];
	XrInputPose controllerAimPoses[HAND_COUNT];
	glm::mat4x4 controllerGripPoseMatrixes[HAND_COUNT];
	XrInputPose controllerGripPoses[HAND_COUNT];

	// [tdbe] Input States. Also includes lastChangeTime, isActive, changedSinceLastSync properties.
	XrActionStateFloat grabState[HAND_COUNT];
	XrActionStateVector2f thumbStickState[HAND_COUNT];
	XrActionStateBoolean quitClickState[HAND_COUNT];

	// [tdbe] Headset State. Use to detect status / user proximity / user presence / user engagement https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#session-lifecycle
	XrSessionState headsetActivityState = XR_SESSION_STATE_UNKNOWN;
};

#endif

class Renderer;

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

#ifdef XR_SUPPORT
	static XrInputData xr_data;
#endif

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

#ifdef XR_SUPPORT
	// static glm::vec2 get_axis(uint8_t button) { return buttons[button] == GLFW_PRESS; }
#endif
};