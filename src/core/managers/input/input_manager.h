#pragma once

#include "core/managers/manager.h"

#include "GLFW/glfw3.h"

#include "glm/ext/quaternion_float.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#define KEYS_SIZE GLFW_KEY_LAST + 1

struct GLFWwindow;

class InputManager : public Manager {
    MANAGER_DECLARE(InputManager)
    friend class SimulationManager;

public:
    enum eMouseButton {
        MOUSE_LEFT,
        MOUSE_RIGHT,
        MOUSE_MIDDLE,
        MOUSE_BUTTONS_SIZE
    };

    // https://www.glfw.org/docs/3.3/group__keys.html
    bool is_key_pressed(int key, bool stop_propagation = false);
    bool was_key_pressed(int key, bool stop_propagation = false);

    // https://www.glfw.org/docs/3.3/group__buttons.html
    bool is_mouse_pressed(uint8_t button);
    bool was_mouse_pressed(uint8_t button);
    bool was_mouse_released(uint8_t button);
    glm::vec2 get_mouse_position();
    glm::vec2 get_mouse_delta();
    float get_mouse_wheel_delta();

    void mouse_center();

private:
    InputManager() = default;
    ~InputManager() = default;

    Error initialize() override;
    Error finalize() override;

    void update_mouse();

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void mouse_scroll_callback(GLFWwindow* window, double offset_x, double offset_y);

    void set_key_state(int key, uint8_t value);
    void set_mouse_button(int button, uint8_t value);
    void set_mouse_wheel(float offset_x, float offset_y);

    void set_prev_state();

    // Mouse state
    glm::vec2 mouse_position = {};
    glm::vec2 mouse_delta = {};
    float mouse_wheel_delta = {};
    uint8_t mouse_buttons[MOUSE_BUTTONS_SIZE];
    uint8_t prev_mouse_buttons[MOUSE_BUTTONS_SIZE];

    // Keyboard
    uint8_t keystate[KEYS_SIZE];
    uint8_t prev_keystate[KEYS_SIZE];
};
