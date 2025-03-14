#include "input.h"
#include "graphics/renderer.h"

#ifdef XR_SUPPORT
#include "framework/math/transform.h"
#endif

#if defined(OPENXR_SUPPORT)
#include "xr/openxr/openxr_context.h"
#elif defined(WEBXR_SUPPORT)
#include "xr/webxr/webxr_context.h"
#endif

#include <glm/gtc/type_ptr.hpp>

glm::vec2 Input::mouse_position; //last mouse position
glm::vec2 Input::mouse_delta; //mouse movement in the last frame
float Input::mouse_wheel_delta = 0.0f;
uint8_t Input::buttons[GLFW_MOUSE_BUTTON_LAST];
uint8_t Input::prev_buttons[GLFW_MOUSE_BUTTON_LAST];

uint8_t Input::keystate[GLFW_KEY_LAST];
uint8_t Input::prev_keystate[GLFW_KEY_LAST];

bool Input::use_glfw = false;

#ifdef XR_SUPPORT
bool Input::prev_trigger_state[HAND_COUNT] = { false, false };
bool Input::prev_grab_state[HAND_COUNT] = { false, false };
XRContext* Input::xr_context = nullptr;
#endif

GLFWwindow* Input::window = nullptr;
bool Input::use_mirror_screen;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key < 0) return;

    Input::set_key_state(key, action != GLFW_RELEASE);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Input::set_mouse_button(button, action != GLFW_RELEASE);
}

void mouse_scroll_callback(GLFWwindow* window, double offset_x, double offset_y)
{
    Input::set_mouse_wheel(static_cast<float>(offset_x), static_cast<float>(offset_y));
}

void Input::init(GLFWwindow* _window, bool use_mirror_screen, bool use_glfw)
{
    Input::use_glfw = use_glfw;
    Input::use_mirror_screen = use_mirror_screen;

    window = _window;

    if (use_glfw) {
        glfwSetKeyCallback(_window, key_callback);
        glfwSetMouseButtonCallback(_window, mouse_button_callback);
        glfwSetScrollCallback(_window, mouse_scroll_callback);
    }

	if (use_mirror_screen) {
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		Input::mouse_position.x = static_cast<float>(x);
		Input::mouse_position.y = static_cast<float>(y);
	}
}

bool Input::init_xr(XRContext* context)
{
    if (!context) {
		return 1;
    }

#if defined(OPENXR_SUPPORT)

    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(context);
	XrInstance* instance = openxr_context->get_instance();

    // Add mapped buttons using enum order (input.h).
    XrMappedButtonState mb{ .name = "button_a", .hand = HAND_RIGHT };
    mb.bind_click(instance, "/user/hand/right/input/a/click");
    mb.bind_touch(instance, "/user/hand/right/input/a/touch");
    openxr_context->buttonsState.push_back(mb);

    mb = { .name = "button_b", .hand = HAND_RIGHT };
    mb.bind_click(instance, "/user/hand/right/input/b/click");
    mb.bind_touch(instance, "/user/hand/right/input/b/touch");
    openxr_context->buttonsState.push_back(mb);

    mb = { .name = "button_x", .hand = HAND_LEFT };
    mb.bind_click(instance, "/user/hand/left/input/x/click");
    mb.bind_touch(instance, "/user/hand/left/input/x/touch");
    openxr_context->buttonsState.push_back(mb);

    mb = { .name = "button_y", .hand = HAND_LEFT };
    mb.bind_click(instance, "/user/hand/left/input/y/click");
    mb.bind_touch(instance, "/user/hand/left/input/y/touch");
    openxr_context->buttonsState.push_back(mb);

    mb = { .name = "button_menu", .hand = HAND_LEFT };
    mb.bind_click(instance, "/user/hand/left/input/menu/click");
    openxr_context->buttonsState.push_back(mb);

    openxr_context->init_actions();

    xr_context = openxr_context;

#elif defined(WEBXR_SUPPORT)

    WebXRContext* webr_context = static_cast<WebXRContext*>(context);
    webr_context->buttonsState.resize(XR_BUTTON_COUNT);
    xr_context = webr_context;

#endif

    return 0;
}

void Input::update(float delta_time)
{
    glfwPollEvents();

    // Mouse  state
    if (use_glfw && window) {

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        mouse_delta.x = mouse_position.x - static_cast<float>(x);
        mouse_delta.y = mouse_position.y - static_cast<float>(y);

        mouse_position.x = static_cast<float>(x);
        mouse_position.y = static_cast<float>(y);
    }

#ifdef OPENXR_SUPPORT
    // Sync XR Controllers
    if (!xr_context)
        return;

    xr_context->poll_actions();
#endif
}

void Input::center_mouse()
{
    if (!use_mirror_screen)
        return;

    int screen_width, screen_height;
    glfwGetWindowSize(window, &screen_width, &screen_height);

    int center_x = (int)floor(screen_width * 0.5f);
    int center_y = (int)floor(screen_height * 0.5f);
    glfwSetCursorPos(window, center_x, center_y);
    mouse_position.x = (float)center_x;
    mouse_position.y = (float)center_y;
}

bool Input::is_key_pressed(int key, bool stop_propagation)
{
    bool pressed = (keystate[key] == GLFW_PRESS);
    if (stop_propagation) {
        keystate[key] = GLFW_RELEASE;
    }
    return pressed;
}

bool Input::was_key_pressed(int key, bool stop_propagation)
{
    bool pressed = (prev_keystate[key] == GLFW_RELEASE && keystate[key] == GLFW_PRESS);
    if (stop_propagation) {
        keystate[key] = GLFW_RELEASE;
    }
    return pressed;
}

void Input::set_prev_state()
{
    memcpy((void*)&prev_keystate, keystate, GLFW_KEY_LAST);
    memcpy((void*)&prev_buttons, buttons, GLFW_MOUSE_BUTTON_LAST);

#ifdef XR_SUPPORT
    for (int i = 0; i < HAND_COUNT; ++i) {
        prev_trigger_state[i] = is_trigger_pressed(i);
        prev_grab_state[i] = is_grab_pressed(i);
    }
#endif
}

void Input::set_key_state(int key, uint8_t value)
{
    keystate[key] = value;
}

void Input::set_mouse_button(int button, uint8_t value)
{
    buttons[button] = value;
}

void Input::set_mouse_wheel(float offset_x, float offset_y)
{
    mouse_wheel_delta = offset_y;
}

glm::mat4x4 Input::get_controller_pose(uint8_t controller, uint8_t type, bool world_space)
{
#ifdef XR_SUPPORT
    if (!xr_context) return glm::mat4x4(1.0f);
    glm::mat4 mat;
    if (type == POSE_AIM) mat = xr_context->controllerAimPoseMatrices[controller];
    else mat = xr_context->controllerGripPoseMatrices[controller];

    if (xr_context->root_transform && world_space) {
        return (xr_context->root_transform->get_model() * mat);
    }

    return mat;
#else
    return glm::mat4x4(1.f);
#endif
}

glm::vec3 Input::get_controller_position(uint8_t controller, uint8_t type, bool world_space)
{
#ifdef XR_SUPPORT
    if (!xr_context) return {};
    glm::vec3 pos;
    if (type == POSE_AIM) pos = xr_context->controllerAimPoses[controller].position;
    else pos = xr_context->controllerGripPoses[controller].position;

    if (xr_context->root_transform && world_space) {
        return (xr_context->root_transform->get_model() * glm::vec4(pos, 1.0f));
    }
    return pos;
#else
    return {};
#endif
}

glm::quat Input::get_controller_rotation(uint8_t controller, uint8_t type)
{
#ifdef XR_SUPPORT
    if (!xr_context) return { 0.0f, 0.0f, 0.0f, 1.0f };
    if (type == POSE_AIM) return xr_context->controllerAimPoses[controller].orientation;
    else return xr_context->controllerGripPoses[controller].orientation;
#else
    return {};
#endif
}

bool Input::is_button_pressed(uint8_t button)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && openxr_context->buttonsState[button].click.state.currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && webxr_context->buttonsState[button].pressed;
#else
    return false;
#endif
}

bool Input::was_button_pressed(uint8_t button)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && (openxr_context->buttonsState[button].click.state.currentState && openxr_context->buttonsState[button].click.state.changedSinceLastSync);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && (webxr_context->buttonsState[button].pressed && webxr_context->buttonsState[button].changedSinceLastSync[GAMEPAD_BUTTON_PRESSED_STATE]);
#else
    return false;
#endif
}

bool Input::was_button_released(uint8_t button)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && (!openxr_context->buttonsState[button].click.state.currentState && openxr_context->buttonsState[button].click.state.changedSinceLastSync);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && (!webxr_context->buttonsState[button].pressed && webxr_context->buttonsState[button].changedSinceLastSync[GAMEPAD_BUTTON_PRESSED_STATE]);
#else
    return false;
#endif
}

bool Input::is_button_touched(uint8_t button)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && openxr_context->buttonsState[button].touch.state.currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && webxr_context->buttonsState[button].touched;
#else
    return false;
#endif
}

bool Input::was_button_touched(uint8_t button)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && (openxr_context->buttonsState[button].touch.state.currentState && openxr_context->buttonsState[button].touch.state.changedSinceLastSync);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && (webxr_context->buttonsState[button].touched && webxr_context->buttonsState[button].changedSinceLastSync[GAMEPAD_BUTTON_TOUCHED_STATE]);
#else
    return false;
#endif
}

/*
*	Triggers
*/

float Input::get_trigger_value(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    if (!openxr_context) return 0.0f;
    return openxr_context->triggerValueState[controller].currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    if (!webxr_context) return 0.0f;
    return webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].value;
#else
    return 0.0f;
#endif
}

bool Input::is_trigger_pressed(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && openxr_context->triggerValueState[controller].currentState > 0.5f;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].value > 0.5f;
#else
    return false;
#endif
}

bool Input::was_trigger_pressed(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && !prev_trigger_state[controller] && (openxr_context->triggerValueState[controller].currentState > 0.5f);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && !prev_trigger_state[controller] && webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].value > 0.5f;
#else
    return false;
#endif
}

bool Input::was_trigger_released(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && prev_trigger_state[controller] && (openxr_context->triggerValueState[controller].currentState < 0.5f);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && prev_trigger_state[controller] && webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].value < 0.5f;
#else
    return false;
#endif
}

bool Input::is_trigger_touched(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && openxr_context->triggerTouchState[controller].currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].touched;
#else
    return false;
#endif
}

bool Input::was_trigger_touched(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && (openxr_context->triggerTouchState[controller].currentState && openxr_context->triggerTouchState[controller].changedSinceLastSync);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && (webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].touched && webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].changedSinceLastSync[GAMEPAD_BUTTON_TOUCHED_STATE]);
#else
    return false;
#endif
}

/*
*	Grabs
*/

bool Input::is_grab_pressed(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && openxr_context->grabState[controller].currentState > 0.5f;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && webxr_context->handButtons[controller][WEBXR_BUTTON_GRAB].value > 0.5f;
#else
    return false;
#endif
}

bool Input::was_grab_pressed(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && !prev_grab_state[controller] && (openxr_context->grabState[controller].currentState > 0.5f);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && !prev_grab_state[controller] && webxr_context->handButtons[controller][WEBXR_BUTTON_GRAB].value > 0.5f;
#else
    return false;
#endif
}

bool Input::was_grab_released(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && prev_grab_state[controller] && (openxr_context->grabState[controller].currentState < 0.5f);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && prev_grab_state[controller] && webxr_context->handButtons[controller][WEBXR_BUTTON_GRAB].value < 0.5f;
#else
    return false;
#endif
}

float Input::get_grab_value(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    if (!openxr_context) return 0.0f;
    return openxr_context->grabState[controller].currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    if (!webxr_context) return 0.0f;
    return webxr_context->handButtons[controller][WEBXR_BUTTON_GRAB].value;
#else
    return 0.0f;
#endif
}

/*
*	Thumbsticks
*/

glm::vec2 Input::get_thumbstick_value(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    if (!openxr_context) return { 0.0f, 0.0f };
    return glm::make_vec2(&openxr_context->thumbStickValueState[controller].currentState.x);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    if (!webxr_context) return { 0.0f, 0.0f };
    return webxr_context->axisState[controller];
#else
    return { 0.0f, 0.0f };
#endif
}

bool Input::is_thumbstick_pressed(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && openxr_context->thumbStickClickState[controller].currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && webxr_context->handButtons[controller][WEBXR_BUTTON_THUMBSTICK_PRESS].pressed;
#else
    return false;
#endif
}

bool Input::was_thumbstick_pressed(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && (openxr_context->thumbStickClickState[controller].currentState && openxr_context->thumbStickClickState[controller].changedSinceLastSync);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && (webxr_context->handButtons[controller][WEBXR_BUTTON_THUMBSTICK_PRESS].pressed && webxr_context->handButtons[controller][WEBXR_BUTTON_THUMBSTICK_PRESS].changedSinceLastSync[GAMEPAD_BUTTON_PRESSED_STATE]);
#else
    return false;
#endif
}

bool Input::is_thumbstick_touched(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && openxr_context->thumbStickTouchState[controller].currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && webxr_context->handButtons[controller][WEBXR_BUTTON_THUMBSTICK_PRESS].touched;
#else
    return false;
#endif
}

bool Input::was_thumbstick_touched(uint8_t controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context && (openxr_context->thumbStickTouchState[controller].currentState && openxr_context->thumbStickTouchState[controller].changedSinceLastSync);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context && (webxr_context->handButtons[controller][WEBXR_BUTTON_THUMBSTICK_PRESS].touched && webxr_context->handButtons[controller][WEBXR_BUTTON_THUMBSTICK_PRESS].changedSinceLastSync[GAMEPAD_BUTTON_TOUCHED_STATE]);
#else
    return false;
#endif
}

uint8_t Input::get_leading_thumbstick_axis(uint8_t controller)
{
#ifdef XR_SUPPORT
    const glm::vec2& value = Input::get_thumbstick_value(controller);
    const glm::vec2& abs_axis = glm::abs(value);
    if (glm::abs(glm::length(abs_axis)) >= XR_THUMBSTICK_DEADZONE) {
        if (abs_axis.x > abs_axis.y) {
            return XR_THUMBSTICK_AXIS_X;
        }
        else {
            return XR_THUMBSTICK_AXIS_Y;
        }
    }
#endif
    return XR_THUMBSTICK_NO_AXIS;
}
