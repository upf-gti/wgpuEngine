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

OpenXRContext* opexr_context = nullptr;

void Input::init(GLFWwindow* _window, Renderer* renderer)
{	
	use_mirror_screen = renderer->get_use_mirror_screen();

	opexr_context = renderer->get_openxr_context();

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

	// Sync XR Controllers
	{
		if (!opexr_context)
			return;

		opexr_context->poll_actions();

		std::cout << opexr_context->input_data.grabState[1].currentState << std::endl;
	}
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