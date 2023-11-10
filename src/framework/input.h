#pragma once

#include "includes.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

// Small helper so we don't forget whether we treat 0 as left or right hand
enum OPENXR_HANDS
{
	HAND_LEFT = 0,
	HAND_RIGHT = 1,
	HAND_COUNT
};

enum {
	POSE_GRIP = 0,
	POSE_AIM
};

enum {
	XR_BUTTON_A = 0,
	XR_BUTTON_B,
	XR_BUTTON_X,
	XR_BUTTON_Y,
	XR_BUTTON_MENU,
};

#ifdef XR_SUPPORT

#include "dawnxr/dawnxr.h"
#include "openxr/openxr_platform.h"

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

class Renderer;
struct OpenXRContext;

class Input {

	// Mouse state
	static glm::vec2 mouse_position; //last mouse position
	static glm::vec2 mouse_delta; //mouse movement in the last frame
	static float mouse_wheel;
	static float mouse_wheel_delta;
	static uint8_t buttons[GLFW_MOUSE_BUTTON_LAST];
	static uint8_t prev_buttons[GLFW_MOUSE_BUTTON_LAST];

    static bool use_glfw;

	// Keyboard
	static uint8_t keystate[GLFW_KEY_LAST];
	static uint8_t prev_keystate[GLFW_KEY_LAST];

	static GLFWwindow* window;

	static bool use_mirror_screen;

    static bool trigger_released[HAND_COUNT];

#ifdef XR_SUPPORT
	static XrInputData xr_data;

	/*
	*	Conversors
	*/
	
	static bool XrBool32_to_bool(XrBool32 v) { return static_cast<bool>(v); }
	static glm::vec2 XrVector2f_to_glm(XrVector2f v) { return glm::vec2(v.x, v.y); }
#endif

public:

	static void init(GLFWwindow* window, Renderer* renderer, bool use_glfw);
	static void update(float delta_time);
	static void center_mouse();

	// https://www.glfw.org/docs/3.3/group__keys.html
	static bool is_key_pressed(int key) { return keystate[key] == GLFW_PRESS; }
	static bool was_key_pressed(int key) { return prev_keystate[key] == GLFW_RELEASE && keystate[key] == GLFW_PRESS; }

	// https://www.glfw.org/docs/3.3/group__buttons.html
	static bool is_mouse_pressed(uint8_t button) { return buttons[button] == GLFW_PRESS; }
	static bool was_mouse_pressed(uint8_t button) { return prev_buttons[button] == GLFW_RELEASE && buttons[button] == GLFW_PRESS; }
    static glm::vec2 get_mouse_delta() { return mouse_delta; }

    static void set_key_state(int key, uint8_t value);
    static void set_mouse_button(int button, uint8_t value);

#ifdef XR_SUPPORT
	static bool init_xr(OpenXRContext* context);
#endif

	/*
	*	Poses
	*/

	static glm::vec3 get_controller_position(uint8_t controller, uint8_t type = POSE_GRIP);
	static glm::quat get_controller_rotation(uint8_t controller, uint8_t type = POSE_GRIP);
	static glm::mat4x4 get_controller_pose(uint8_t controller, uint8_t type = POSE_GRIP);

	/*
	*	Buttons
	*/

	static bool is_button_pressed(uint8_t button);
	static bool was_button_pressed(uint8_t button);
	static bool was_button_released(uint8_t button);
	static bool is_button_touched(uint8_t button);
	static bool was_button_touched(uint8_t button);

	/*
	*	Grabs
	*/

	static float get_grab_value(uint8_t controller);

	/*
	*	Triggers
	*/

	static float get_trigger_value(uint8_t controller);
    static bool was_trigger_pressed(uint8_t controller);
	static bool is_trigger_touched(uint8_t controller);
	static bool was_trigger_touched(uint8_t controller);

	/*
	*	Thumbsticks
	*/

	static glm::vec2 get_thumbstick_value(uint8_t controller);
	static bool is_thumbstick_pressed(uint8_t controller);
	static bool was_thumbstick_pressed(uint8_t controller);
	static bool is_thumbstick_touched(uint8_t controller);
	static bool was_thumbstick_touched(uint8_t controller);
};
