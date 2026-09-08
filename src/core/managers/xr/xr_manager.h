#pragma once

#include "core/managers/manager.h"
#include "xr/xr_context.h"

class XRManager : public Manager {
    MANAGER_DECLARE(XRManager)
    friend class SimulationManager;
    friend class RenderStorage;
    friend class RenderAPI;
    friend class RenderMethod;

public:
    bool is_xr_available() const;

    void vibrate_hand(int controller, float amplitude, float duration);

    void set_thumbstick_deadzone(float deadzone) { xr_thumbstick_deadzone = deadzone; }
    float get_thumbstick_deadzone() const { return xr_thumbstick_deadzone; }

    WGPUTextureFormat get_swapchain_format();
    WGPUTextureView get_swapchain_view(uint8_t eye_idx, uint32_t image_idx);

    uint32_t get_num_images_per_swapchain();

    // Poses
    glm::mat4x4 get_controller_pose(eXRHand controller, uint8_t type = POSE_GRIP, bool world_space = true);
    glm::vec3 get_controller_position(eXRHand controller, uint8_t type = POSE_GRIP, bool world_space = true);
    glm::quat get_controller_rotation(eXRHand controller, uint8_t type = POSE_GRIP);

    // Buttons
    bool is_button_pressed(uint8_t button);
    bool was_button_pressed(uint8_t button);
    bool was_button_released(uint8_t button);
    bool is_button_touched(uint8_t button);
    bool was_button_touched(uint8_t button);

    // Grabs
    bool is_grab_pressed(eXRHand controller);
    bool was_grab_pressed(eXRHand controller);
    bool was_grab_released(eXRHand controller);
    float get_grab_value(eXRHand controller);

    // Triggers
    float get_trigger_value(eXRHand controller);
    bool is_trigger_pressed(eXRHand controller);
    bool was_trigger_pressed(eXRHand controller);
    bool was_trigger_released(eXRHand controller);
    bool is_trigger_touched(eXRHand controller);
    bool was_trigger_touched(eXRHand controller);

    // Triggers
    uint8_t get_leading_thumbstick_axis(eXRHand controller);
    glm::vec2 get_thumbstick_value(eXRHand controller);
    bool is_thumbstick_pressed(eXRHand controller);
    bool was_thumbstick_pressed(eXRHand controller);
    bool is_thumbstick_touched(eXRHand controller);
    bool was_thumbstick_touched(eXRHand controller);

private:
    XRManager() = default;
    ~XRManager() = default;

    Error initialize() override;
    Error finalize() override;

    Error create_xr_instance();
    Error create_xr_context();

    void poll_actions();

    XRContext* xr_context = nullptr;

    // XR controllers
    bool prev_trigger_state[HAND_COUNT];
    bool prev_grab_state[HAND_COUNT];

    float xr_thumbstick_deadzone = 0.01f;

    void set_prev_state();

    Error initialize_inputs();
};
