#pragma once

#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include <vector>

class Renderer;

struct XrInputData {
    /*
    // [tdbe] Poses
    std::vector<glm::mat4> eyePoseMatrixes;//(int)SideEnum::COUNT
    std::vector<util::posef> eyePoses;//(int)SideEnum::COUNT
    glm::mat4 headPoseMatrix;
    util::posef headPose;
    std::vector<glm::mat4> controllerAimPoseMatrixes;//(int)ControllerEnum::COUNT
    std::vector<util::posef> controllerAimPoses;
    std::vector<glm::mat4> controllerGripPoseMatrixes;
    std::vector<util::posef> controllerGripPoses;

    // [tdbe] Input States. Also includes lastChangeTime, isActive, changedSinceLastSync properties.
    std::vector<XrActionStateFloat> grabState{XR_TYPE_ACTION_STATE_FLOAT};
    std::vector<XrActionStateVector2f> thumbStickState{XR_TYPE_ACTION_STATE_VECTOR2F};
    std::vector<XrActionStateBoolean> menuClickState{XR_TYPE_ACTION_STATE_BOOLEAN};
    std::vector<XrActionStateBoolean> selectClickState{XR_TYPE_ACTION_STATE_BOOLEAN};
    std::vector<XrActionStateFloat> triggerState{XR_TYPE_ACTION_STATE_BOOLEAN};


    // [tdbe] Headset State. Use to detect status / user proximity / user presence / user engagement https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#session-lifecycle
    XrSessionState headsetActivityState = XR_SESSION_STATE_UNKNOWN;

    void SizeVectors(ControllerEnum controllers, SideEnum sides) {
        eyePoseMatrixes.resize((int)sides);
        eyePoses.resize((int)sides);

        controllerAimPoseMatrixes.resize((int)controllers);
        controllerAimPoses.resize((int)controllers);
        controllerGripPoseMatrixes.resize((int)controllers);
        controllerGripPoses.resize((int)controllers);
        grabState.resize((int)controllers);
        thumbStickState.resize((int)controllers);
        menuClickState.resize((int)controllers);
        selectClickState.resize((int)controllers);
        triggerState.resize((int)controllers);
    }
    */
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