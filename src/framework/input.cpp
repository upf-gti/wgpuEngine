#include "input.h"
#include "graphics/renderer.h"

#ifdef XR_SUPPORT
#include "xr/openxr_context.h"
#endif

glm::vec2 Input::mouse_position; //last mouse position
glm::vec2 Input::mouse_delta; //mouse movement in the last frame
float Input::mouse_wheel_delta = 0.0f;
uint8_t Input::buttons[GLFW_MOUSE_BUTTON_LAST];
uint8_t Input::prev_buttons[GLFW_MOUSE_BUTTON_LAST];

uint8_t Input::keystate[GLFW_KEY_LAST];
uint8_t Input::prev_keystate[GLFW_KEY_LAST];

bool Input::use_glfw = false;

#ifdef XR_SUPPORT

bool Input::trigger_released[HAND_COUNT] = {true, true};
bool Input::grab_released[HAND_COUNT] = {true, true};

#endif

GLFWwindow* Input::window = nullptr;
bool Input::use_mirror_screen;

#ifdef XR_SUPPORT
XrInputData Input::xr_data;
OpenXRContext* openxr_context = nullptr;
#endif

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

void Input::init(GLFWwindow* _window, Renderer* renderer, bool use_glfw)
{
    Input::use_glfw = use_glfw;
	use_mirror_screen = renderer->get_use_mirror_screen();
    window = _window;

    if (use_glfw) {
        glfwSetKeyCallback(_window, key_callback);
        glfwSetMouseButtonCallback(_window, mouse_button_callback);
        glfwSetScrollCallback(_window, mouse_scroll_callback);
    }

	if (use_mirror_screen)
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		Input::mouse_position.x = static_cast<float>(x);
		Input::mouse_position.y = static_cast<float>(y);
	}
}

#ifdef XR_SUPPORT
bool Input::init_xr(OpenXRContext* context)
{
	if (!context)
		return 1;

	openxr_context = context;

	XrInstance* instance = openxr_context->get_instance();

	// Add mapped buttons using enum order (input.h).
	XrMappedButtonState mb{ .name = "button_a", .hand = HAND_RIGHT };
	mb.bind_click(instance, "/user/hand/right/input/a/click");
	mb.bind_touch(instance, "/user/hand/right/input/a/touch");
	xr_data.buttonsState.push_back(mb);

	mb = { .name = "button_b", .hand = HAND_RIGHT };
	mb.bind_click(instance, "/user/hand/right/input/b/click");
	mb.bind_touch(instance, "/user/hand/right/input/b/touch");
	xr_data.buttonsState.push_back(mb);

	mb = { .name = "button_x", .hand = HAND_LEFT };
	mb.bind_click(instance, "/user/hand/left/input/x/click");
	mb.bind_touch(instance, "/user/hand/left/input/x/touch");
	xr_data.buttonsState.push_back(mb);

	mb = { .name = "button_y", .hand = HAND_LEFT };
	mb.bind_click(instance, "/user/hand/left/input/y/click");
	mb.bind_touch(instance, "/user/hand/left/input/y/touch");
	xr_data.buttonsState.push_back(mb); 

	mb = { .name = "button_menu", .hand = HAND_LEFT };
	mb.bind_click(instance, "/user/hand/left/input/menu/click");
	xr_data.buttonsState.push_back(mb);

	openxr_context->init_actions(xr_data);

	return 0;
}
#endif

void Input::update(float delta_time)
{
    glfwPollEvents();

	// Mouse  state
	if (use_glfw && window) {

		double x, y;
		glfwGetCursorPos(window, &x, &y);
		Input::mouse_delta.x = Input::mouse_position.x - static_cast<float>(x);
		Input::mouse_delta.y = Input::mouse_position.y - static_cast<float>(y);

		Input::mouse_position.x = static_cast<float>(x);
		Input::mouse_position.y = static_cast<float>(y);
	}
	
#ifdef XR_SUPPORT

	// Sync XR Controllers
	if (!openxr_context)
		return;

	openxr_context->poll_actions(xr_data);

    for (int i = 0; i < HAND_COUNT; ++i)
    {
        if (get_trigger_value(i) == 0.f) {
            trigger_released[i] = true;
        }

        if (get_grab_value(i) == 0.f) {
            grab_released[i] = true;
        }
    }

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
	Input::mouse_position.x = (float)center_x;
	Input::mouse_position.y = (float)center_y;
}

void Input::set_prev_state()
{
    memcpy((void*)&Input::prev_keystate, Input::keystate, GLFW_KEY_LAST);
    memcpy((void*)&Input::prev_buttons, Input::buttons, GLFW_MOUSE_BUTTON_LAST);
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

glm::vec3 Input::get_controller_position(uint8_t controller, uint8_t type)
{
#ifdef XR_SUPPORT
	if (!openxr_context) return {};
	if (type == POSE_AIM) return xr_data.controllerAimPoses[controller].position;
	else return xr_data.controllerGripPoses[controller].position;
#else
	return {};
#endif
}

glm::quat Input::get_controller_rotation(uint8_t controller, uint8_t type)
{
#ifdef XR_SUPPORT
	if (!openxr_context) return { 0.0f, 0.0f, 0.0f, 1.0f };
	if (type == POSE_AIM) return xr_data.controllerAimPoses[controller].orientation;
	else return xr_data.controllerGripPoses[controller].orientation;
#else
	return {};
#endif
}

glm::mat4x4 Input::get_controller_pose(uint8_t controller, uint8_t type)
{
#ifdef XR_SUPPORT
    if (!openxr_context) return glm::mat4x4(1.0f);
	if (type == POSE_AIM) return xr_data.controllerAimPoseMatrices[controller];
	else return xr_data.controllerGripPoseMatrices[controller];
#else
	return glm::mat4x4(1.f);
#endif
}

bool Input::is_button_pressed(uint8_t button)
{
#ifdef XR_SUPPORT
	return openxr_context && xr_data.buttonsState[button].click.state.currentState;
#else
	return false;
#endif
}

bool Input::was_button_pressed(uint8_t button)
{
#ifdef XR_SUPPORT
	return openxr_context && (xr_data.buttonsState[button].click.state.currentState && xr_data.buttonsState[button].click.state.changedSinceLastSync);
#else
	return false;
#endif
}

bool Input::was_button_released(uint8_t button)
{
#ifdef XR_SUPPORT
	return openxr_context && (!xr_data.buttonsState[button].click.state.currentState && xr_data.buttonsState[button].click.state.changedSinceLastSync);
#else
	return false;
#endif
}

bool Input::is_button_touched(uint8_t button)
{
#ifdef XR_SUPPORT
	return openxr_context && xr_data.buttonsState[button].touch.state.currentState;
#else
	return false;
#endif
}

bool Input::was_button_touched(uint8_t button)
{
#ifdef XR_SUPPORT
	return openxr_context && (xr_data.buttonsState[button].touch.state.currentState && xr_data.buttonsState[button].touch.state.changedSinceLastSync);
#else
	return false;
#endif
}

/*
*	Triggers
*/

float Input::get_trigger_value(uint8_t controller)
{ 
#ifdef XR_SUPPORT
	if (!openxr_context) return 0.0f;
	return xr_data.triggerValueState[controller].currentState;
#else
	return 0.0f;
#endif
}

bool Input::was_trigger_pressed(uint8_t controller)
{
#ifdef XR_SUPPORT

    bool value = openxr_context && trigger_released[controller] && (xr_data.triggerValueState[controller].currentState > 0.5f);
    if (value) trigger_released[controller] = false;
    return value;

#else
    return false;
#endif
}

bool Input::is_trigger_touched(uint8_t controller)
{
#ifdef XR_SUPPORT
    return openxr_context && XrBool32_to_bool(xr_data.triggerTouchState[controller].currentState);
#else
    return false;
#endif
}

bool Input::was_trigger_touched(uint8_t controller) {
#ifdef XR_SUPPORT
    return openxr_context && (XrBool32_to_bool(xr_data.triggerTouchState[controller].currentState) && XrBool32_to_bool(xr_data.triggerTouchState[controller].changedSinceLastSync));
#else
    return false;
#endif
}

/*
*	Grabs
*/

bool Input::is_grab_pressed(uint8_t controller)
{
#ifdef XR_SUPPORT
    return openxr_context && (xr_data.grabState[controller].currentState > 0.5f);
#else
    return false;
#endif
}

bool Input::was_grab_pressed(uint8_t controller)
{
#ifdef XR_SUPPORT
    bool value = openxr_context && grab_released[controller] && (xr_data.grabState[controller].currentState > 0.5f);
    if (value) grab_released[controller] = false;
    return value;
#else
    return false;
#endif
}

float Input::get_grab_value(uint8_t controller)
{
#ifdef XR_SUPPORT
    if (!openxr_context) return 0.0f;
    return xr_data.grabState[controller].currentState;
#else
    return false;
#endif
}

/*
*	Thumbsticks
*/

glm::vec2 Input::get_thumbstick_value(uint8_t controller)
{
#ifdef XR_SUPPORT
	if (!openxr_context) return {};
	return XrVector2f_to_glm(xr_data.thumbStickValueState[controller].currentState);
#else
	return {};
#endif
}

bool Input::is_thumbstick_pressed(uint8_t controller)
{
#ifdef XR_SUPPORT
	return openxr_context && XrBool32_to_bool(xr_data.thumbStickClickState[controller].currentState);
#else
	return false;
#endif
}

bool Input::was_thumbstick_pressed(uint8_t controller)
{
#ifdef XR_SUPPORT
	return openxr_context && (XrBool32_to_bool(xr_data.thumbStickClickState[controller].currentState) && XrBool32_to_bool(xr_data.thumbStickClickState[controller].changedSinceLastSync));
#else
	return false;
#endif
}

bool Input::is_thumbstick_touched(uint8_t controller)
{ 
#ifdef XR_SUPPORT
	return openxr_context && XrBool32_to_bool(xr_data.thumbStickTouchState[controller].currentState);
#else
	return false;
#endif
}

bool Input::was_thumbstick_touched(uint8_t controller)
{
#ifdef XR_SUPPORT
	return openxr_context && (XrBool32_to_bool(xr_data.thumbStickTouchState[controller].currentState) && XrBool32_to_bool(xr_data.thumbStickTouchState[controller].changedSinceLastSync));
#else
	return false;
#endif
}
