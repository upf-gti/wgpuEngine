#include "input.h"
#include "graphics/renderer.h"

glm::vec2 Input::mouse_position; //last mouse position
glm::vec2 Input::mouse_delta; //mouse movement in the last frame
float Input::mouse_wheel;
float Input::mouse_wheel_delta;
uint8_t Input::buttons[GLFW_MOUSE_BUTTON_LAST];
uint8_t Input::prev_buttons[GLFW_MOUSE_BUTTON_LAST];

uint8_t Input::keystate[GLFW_KEY_LAST];
uint8_t Input::prev_keystate[GLFW_KEY_LAST];

GLFWwindow* Input::window = nullptr;
bool Input::use_mirror_screen;

#ifdef XR_SUPPORT
XrInputData Input::xr_data;
OpenXRContext* openxr_context = nullptr;
#endif

void Input::init(GLFWwindow* _window, Renderer* renderer)
{	
	use_mirror_screen = renderer->get_use_mirror_screen();

	if (use_mirror_screen)
	{
		double x, y;
		window = _window;
		glfwGetCursorPos(window, &x, &y);
		Input::mouse_position.x = x;
		Input::mouse_position.y = y;
		mouse_wheel = 0.0;
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
	if (use_mirror_screen)
	{
		// Mouse  state
		{
			double x, y;
			glfwGetCursorPos(window, &x, &y);
			Input::mouse_delta.x = Input::mouse_position.x - x;
			Input::mouse_delta.y = Input::mouse_position.y - y;
			Input::mouse_position.x = x;
			Input::mouse_position.y = y;

			memcpy((void*)&Input::prev_buttons, Input::buttons, GLFW_MOUSE_BUTTON_LAST);
			for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; ++i)
				buttons[i] = glfwGetMouseButton(window, i);
		}

		// Keystate
		{
			memcpy((void*)&Input::prev_keystate, Input::keystate, GLFW_KEY_LAST);
			for (int i = 0; i < GLFW_KEY_LAST; ++i)
				keystate[i] = glfwGetKey(window, i);
		}
	}

#ifdef XR_SUPPORT

	// Sync XR Controllers
	if (!openxr_context)
		return;

	openxr_context->poll_actions(xr_data);
#endif
}

void Input::center_mouse()
{
	if (!use_mirror_screen)
		return;

	int window_width, window_height;
	glfwGetWindowSize(window, &window_width, &window_height);

	int center_x = (int)floor(window_width * 0.5f);
	int center_y = (int)floor(window_height * 0.5f);
	glfwSetCursorPos(window, center_x, center_y);
	Input::mouse_position.x = (float)center_x;
	Input::mouse_position.y = (float)center_y;
}

bool Input::is_button_pressed(uint8_t button)
{
#ifdef XR_SUPPORT
	return xr_data.buttonsState[button].click.state.currentState; 
#else
	return false;
#endif
}

bool Input::was_button_pressed(uint8_t button)
{
#ifdef XR_SUPPORT
	return (xr_data.buttonsState[button].click.state.currentState && xr_data.buttonsState[button].click.state.changedSinceLastSync);
#else
	return false;
#endif
}

bool Input::is_button_touched(uint8_t button)
{
#ifdef XR_SUPPORT
	return xr_data.buttonsState[button].touch.state.currentState;
#else
	return false;
#endif
}

bool Input::was_button_touched(uint8_t button)
{
#ifdef XR_SUPPORT
	return (xr_data.buttonsState[button].touch.state.currentState && xr_data.buttonsState[button].touch.state.changedSinceLastSync);
#else
	return false;
#endif
}

float Input::get_grab_value(uint8_t controller)
{ 
#ifdef XR_SUPPORT
	return xr_data.grabState[controller].currentState;
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
	return xr_data.triggerValueState[controller].currentState;
#else
	return false;
#endif
}

bool Input::is_trigger_touched(uint8_t controller)
{ 
#ifdef XR_SUPPORT
	return XrBool32_to_bool(xr_data.triggerTouchState[controller].currentState);
#else
	return false;
#endif
}

bool Input::was_trigger_touched(uint8_t controller) {
#ifdef XR_SUPPORT
	return (XrBool32_to_bool(xr_data.triggerTouchState[controller].currentState) && XrBool32_to_bool(xr_data.triggerTouchState[controller].changedSinceLastSync));
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
	return XrVector2f_to_glm(xr_data.thumbStickValueState[controller].currentState);
#else
	return {};
#endif
}

bool Input::is_thumbstick_pressed(uint8_t controller)
{
#ifdef XR_SUPPORT
	return XrBool32_to_bool(xr_data.thumbStickClickState[controller].currentState);
#else
	return false;
#endif
}

bool Input::was_thumbstick_pressed(uint8_t controller)
{
#ifdef XR_SUPPORT
	return (XrBool32_to_bool(xr_data.thumbStickClickState[controller].currentState) && XrBool32_to_bool(xr_data.thumbStickClickState[controller].changedSinceLastSync));
#else
	return false;
#endif
}

bool Input::is_thumbstick_touched(uint8_t controller)
{ 
#ifdef XR_SUPPORT
	return XrBool32_to_bool(xr_data.thumbStickTouchState[controller].currentState);
#else
	return false;
#endif
}

bool Input::was_thumbstick_touched(uint8_t controller)
{
#ifdef XR_SUPPORT
	return (XrBool32_to_bool(xr_data.thumbStickTouchState[controller].currentState) && XrBool32_to_bool(xr_data.thumbStickTouchState[controller].changedSinceLastSync));
#else
	return false;
#endif
}