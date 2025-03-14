#pragma once

#include "includes.h"
#include <vector>
#include <string>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/ext/quaternion_float.hpp"

#include "xr/xr_context.h"

#include <GLFW/glfw3.h>

#define XR_THUMBSTICK_DEADZONE 0.01f

class Renderer;

#ifdef XR_SUPPORT
class XRContext;
#endif

class Input {

    // Mouse state
    static glm::vec2 mouse_position; //last mouse position
    static glm::vec2 mouse_delta; //mouse movement in the last frame
    static float mouse_wheel_delta;
    static uint8_t buttons[GLFW_MOUSE_BUTTON_LAST];
    static uint8_t prev_buttons[GLFW_MOUSE_BUTTON_LAST];

    static bool use_glfw;

    // Keyboard
    static uint8_t keystate[GLFW_KEY_LAST];
    static uint8_t prev_keystate[GLFW_KEY_LAST];

    static GLFWwindow* window;

    static bool use_mirror_screen;

#ifdef XR_SUPPORT
    static XRContext* xr_context;

    static bool prev_trigger_state[HAND_COUNT];
    static bool prev_grab_state[HAND_COUNT];
#endif

public:

    static void init(GLFWwindow* window, bool use_mirror_screen, bool use_glfw);
    static void update(float delta_time);
    static void center_mouse();

    static void set_prev_state();

    // https://www.glfw.org/docs/3.3/group__keys.html
    static bool is_key_pressed(int key, bool stop_propagation = false);
    static bool was_key_pressed(int key, bool stop_propagation = false);

    // https://www.glfw.org/docs/3.3/group__buttons.html
    static bool is_mouse_pressed(uint8_t button) { return buttons[button] == GLFW_PRESS; }
    static bool was_mouse_pressed(uint8_t button) { return prev_buttons[button] == GLFW_RELEASE && buttons[button] == GLFW_PRESS; }
    static bool was_mouse_released(uint8_t button) { return prev_buttons[button] == GLFW_PRESS && buttons[button] == GLFW_RELEASE; }
    static glm::vec2 get_mouse_position() { return mouse_position; }
    static glm::vec2 get_mouse_delta() { return mouse_delta; }
    static float get_mouse_wheel_delta() { return mouse_wheel_delta; }

    static void set_key_state(int key, uint8_t value);
    static void set_mouse_button(int button, uint8_t value);
    static void set_mouse_wheel(float offset_x, float offset_y);

#ifdef XR_SUPPORT
    static bool init_xr(XRContext* context);
#endif

    /*
    *	Poses
    */

    static glm::mat4x4 get_controller_pose(uint8_t controller, uint8_t type = POSE_GRIP, bool world_space = true);
    static glm::vec3 get_controller_position(uint8_t controller, uint8_t type = POSE_GRIP, bool world_space = true);
    static glm::quat get_controller_rotation(uint8_t controller, uint8_t type = POSE_GRIP);

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

    static bool is_grab_pressed(uint8_t controller);
    static bool was_grab_pressed(uint8_t controller);
    static bool was_grab_released(uint8_t controller);
    static float get_grab_value(uint8_t controller);

    /*
    *	Triggers
    */

    static float get_trigger_value(uint8_t controller);
    static bool was_trigger_pressed(uint8_t controller);
    static bool was_trigger_released(uint8_t controller);
    static bool is_trigger_pressed(uint8_t controller);
    static bool is_trigger_touched(uint8_t controller);
    static bool was_trigger_touched(uint8_t controller);

    /*
    *	Thumbsticks
    */

    static uint8_t get_leading_thumbstick_axis(uint8_t controller);
    static glm::vec2 get_thumbstick_value(uint8_t controller);
    static bool is_thumbstick_pressed(uint8_t controller);
    static bool was_thumbstick_pressed(uint8_t controller);
    static bool is_thumbstick_touched(uint8_t controller);
    static bool was_thumbstick_touched(uint8_t controller);
};
