#include "input_manager.h"

#include "core/managers/display/display_manager.h"

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

EM_JS(bool, is_canvas_focused, (), {
    const el = document.activeElement;
    return !(el && (el.tagName == "INPUT" || el.tagName == "TEXTAREA" || el.isContentEditable)) &&
            (el.tagName == "CANVAS");
});
#endif

Error InputManager::initialize()
{
    singleton_instance = this;

    GLFWwindow* window = DisplayManager::get_singleton()->get_window();

    if (window) {
        glfwSetKeyCallback(window, key_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetScrollCallback(window, mouse_scroll_callback);

        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Apply dpi on non Windows platforms (TODO: check linux/mac)
#ifndef _WIN32
        x *= Renderer::instance->get_webgpu_context()->dpi_scale;
        y *= Renderer::instance->get_webgpu_context()->dpi_scale;
#endif

        mouse_position.x = static_cast<float>(x);
        mouse_position.y = static_cast<float>(y);
    }

    return Error::OK;
}

Error InputManager::finalize()
{
    singleton_instance = nullptr;
    return Error::OK;
}

void InputManager::update_mouse()
{
    // Mouse  state
    GLFWwindow* window = DisplayManager::get_singleton()->get_window();
    if (window) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Apply dpi on non Windows platforms (TODO: check linux/mac)
#ifndef _WIN32
        x *= Renderer::instance->get_webgpu_context()->dpi_scale;
        y *= Renderer::instance->get_webgpu_context()->dpi_scale;
#endif

        mouse_delta.x = mouse_position.x - static_cast<float>(x);
        mouse_delta.y = mouse_position.y - static_cast<float>(y);

        mouse_position.x = static_cast<float>(x);
        mouse_position.y = static_cast<float>(y);
    }
}

bool InputManager::is_key_pressed(int key, bool stop_propagation)
{
    bool pressed = (keystate[key] == GLFW_PRESS);
    if (stop_propagation) {
        keystate[key] = GLFW_RELEASE;
    }
    return pressed;
}

bool InputManager::was_key_pressed(int key, bool stop_propagation)
{
    bool pressed = (prev_keystate[key] == GLFW_RELEASE && keystate[key] == GLFW_PRESS);
    if (stop_propagation) {
        keystate[key] = GLFW_RELEASE;
    }
    return pressed;
}

bool InputManager::is_mouse_pressed(uint8_t button)
{
    return mouse_buttons[button] == GLFW_PRESS;
}

bool InputManager::was_mouse_pressed(uint8_t button)
{
    return prev_mouse_buttons[button] == GLFW_RELEASE && mouse_buttons[button] == GLFW_PRESS;
}

bool InputManager::was_mouse_released(uint8_t button)
{
    return prev_mouse_buttons[button] == GLFW_PRESS && mouse_buttons[button] == GLFW_RELEASE;
}

glm::vec2 InputManager::get_mouse_position()
{
    return mouse_position;
}

glm::vec2 InputManager::get_mouse_delta()
{
    return mouse_delta;
}

float InputManager::get_mouse_wheel_delta()
{
    return mouse_wheel_delta;
}

void InputManager::mouse_center()
{
    GLFWwindow* window = DisplayManager::get_singleton()->get_window();
    if (!window) {
        return;
    }

    int32_t center_x = DisplayManager::get_singleton()->window_get_width() / 2;
    int32_t center_y = DisplayManager::get_singleton()->window_get_height() / 2;
    glfwSetCursorPos(window, center_x, center_y);
    mouse_position.x = (float)center_x;
    mouse_position.y = (float)center_y;
}

void InputManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#ifdef __EMSCRIPTEN__
    if (!is_canvas_focused()) {
        // Other stuff have the browser focus, ignore the event
        return;
    }
#endif

    if (key < 0) {
        return;
    }

    InputManager::singleton_instance->set_key_state(key, action != GLFW_RELEASE);
}

void InputManager::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
#ifdef __EMSCRIPTEN__
    if (!is_canvas_focused()) {
        // Other stuff have the browser focus, ignore the event
        return;
    }
#endif

    InputManager::singleton_instance->set_mouse_button(button, action != GLFW_RELEASE);
}

void InputManager::mouse_scroll_callback(GLFWwindow* window, double offset_x, double offset_y)
{
#ifdef __EMSCRIPTEN__
    if (!is_canvas_focused()) {
        // Other stuff have the browser focus, ignore the event
        return;
    }
#endif

    InputManager::singleton_instance->set_mouse_wheel(static_cast<float>(offset_x), static_cast<float>(offset_y));
}

void InputManager::set_key_state(int key, uint8_t value)
{
    keystate[key] = value;
}

void InputManager::set_mouse_button(int button, uint8_t value)
{
    mouse_buttons[button] = value;
}

void InputManager::set_mouse_wheel(float offset_x, float offset_y)
{
    mouse_wheel_delta = offset_y;
}

void InputManager::set_prev_state()
{
    memcpy((void*)&prev_keystate, keystate, KEYS_SIZE);
    memcpy((void*)&prev_mouse_buttons, mouse_buttons, MOUSE_BUTTONS_SIZE);
}
