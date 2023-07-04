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

#ifdef XR_SUPPORT
	openxr_context = renderer->get_openxr_context();

	if (openxr_context)
	{
		// Add mapped buttons using enum order (input.h).
		XrMappedButtonState mb{ .name = "button_a", .hand = HAND_RIGHT };
		mb.click.path = "/user/hand/right/input/a/click";
		mb.touch.path = "/user/hand/right/input/a/touch";
		xr_data.buttonsState.push_back(mb);

		mb = { .name = "button_b", .hand = HAND_RIGHT };
		mb.click.path = "/user/hand/right/input/b/click";
		mb.touch.path = "/user/hand/right/input/b/touch";
		xr_data.buttonsState.push_back(mb);

		mb = { .name = "button_x", .hand = HAND_LEFT };
		mb.click.path = "/user/left/right/input/x/click";
		mb.touch.path = "/user/left/right/input/x/touch";
		xr_data.buttonsState.push_back(mb);

		mb = { .name = "button_y", .hand = HAND_LEFT };
		mb.click.path = "/user/left/right/input/y/click";
		mb.touch.path = "/user/left/right/input/y/touch";
		xr_data.buttonsState.push_back(mb);

		// Init Xr Actions.
		openxr_context->init_actions(xr_data);
	}
	
#endif

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