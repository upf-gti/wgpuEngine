#include "xr_manager.h"

#if defined(OPENXR_SUPPORT)
#include "xr/dawnxr/dawnxr_internal.h"
#include "xr/openxr/openxr_context.h"
#elif defined(WEBXR_SUPPORT)
#include "xr/webxr/webxr_context.h"
#endif

#include "core/managers/render/render_manager.h"

#include "framework/math/transform.h"

#include <glm/gtc/type_ptr.hpp>

#include "spdlog/spdlog.h"

Error XRManager::initialize()
{
    singleton_instance = this;
    return Error::OK;
}

Error XRManager::initialize_inputs()
{
    if (!xr_context) {
        return Error::FAILED;
    }

#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
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

#elif defined(WEBXR_SUPPORT)
    WebXRContext* webr_context = static_cast<WebXRContext*>(xr_context);
    webr_context->buttonsState.resize(XR_BUTTON_COUNT);
#endif

    return Error::OK;
}

Error XRManager::finalize()
{
    singleton_instance = nullptr;

    delete xr_context;
    xr_context = nullptr;

    return Error::OK;
}

Error XRManager::create_xr_instance()
{
    spdlog::info("Creating XR context");

#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = new OpenXRContext();
    if (openxr_context->create_instance() == Error::OK) {
        xr_context = openxr_context;
    } else {
        spdlog::info("OpenXR context not available");
        delete openxr_context;
        return Error::FAILED;
    }
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = new WebXRContext();
    if (webxr_context->query_session_supported()) {
        xr_context = webxr_context;
    } else {
        spdlog::info("WebXR context not available");
        delete webxr_context;
        return Error::FAILED;
    }

#endif

    return Error::OK;
}

Error XRManager::create_xr_context()
{
#ifdef XR_SUPPORT

    if (xr_context && xr_context->initialize() == Error::FAILED) {
        spdlog::error("Could not initialize XR context");
        delete xr_context;
        xr_context = nullptr;
        return Error::FAILED;
    } else {
        RenderManager::get_singleton()->set_render_size(xr_context->viewport.z, xr_context->viewport.w);
    }
#endif

    if (initialize_inputs()) {
        spdlog::error("Can't initialize OpenXR input");
        return Error::FAILED;
    }

    return Error::OK;
}

void XRManager::poll_actions()
{
    xr_context->poll_actions();
}

void XRManager::set_prev_state()
{
#ifdef XR_SUPPORT
    for (int i = 0; i < HAND_COUNT; ++i) {
        prev_trigger_state[i] = is_trigger_pressed(static_cast<eXRHand>(i));
        prev_grab_state[i] = is_grab_pressed(static_cast<eXRHand>(i));
    }
#endif
}

bool XRManager::is_xr_available() const
{
    if (!xr_context) {
        return false;
    }

#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    return openxr_context->instance != nullptr;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    return webxr_context->session_supported;
#endif
}

void XRManager::vibrate_hand(int controller, float amplitude, float duration)
{
#ifdef OPENXR_SUPPORT
    if (xr_context) {
        xr_context->apply_haptics(controller, amplitude, duration);
    }
#endif
}

WGPUTextureFormat XRManager::get_swapchain_format()
{
    return xr_context ? xr_context->swapchain_format : WGPUTextureFormat_Undefined;
}

glm::mat4x4 XRManager::get_controller_pose(eXRHand controller, uint8_t type, bool world_space)
{
#ifdef XR_SUPPORT
    if (!xr_context) {
        return glm::mat4x4(1.0f);
    }
    glm::mat4 mat;
    if (type == POSE_AIM) {
        mat = xr_context->controllerAimPoseMatrices[controller];
    } else {
        mat = xr_context->controllerGripPoseMatrices[controller];
    }

    if (xr_context->root_transform && world_space) {
        return (xr_context->root_transform->get_model() * mat);
    }

    return mat;
#else
    return glm::mat4x4(1.f);
#endif
}

glm::vec3 XRManager::get_controller_position(eXRHand controller, uint8_t type, bool world_space)
{
#ifdef XR_SUPPORT
    if (!xr_context) {
        return {};
    }
    glm::vec3 pos;
    if (type == POSE_AIM) {
        pos = xr_context->controllerAimPoses[controller].position;
    } else {
        pos = xr_context->controllerGripPoses[controller].position;
    }

    if (xr_context->root_transform && world_space) {
        return (xr_context->root_transform->get_model() * glm::vec4(pos, 1.0f));
    }
    return pos;
#else
    return {};
#endif
}

glm::quat XRManager::get_controller_rotation(eXRHand controller, uint8_t type)
{
#ifdef XR_SUPPORT
    if (!xr_context) {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }
    if (type == POSE_AIM) {
        return xr_context->controllerAimPoses[controller].orientation;
    } else {
        return xr_context->controllerGripPoses[controller].orientation;
    }
#else
    return {};
#endif
}

bool XRManager::is_button_pressed(uint8_t button)
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

bool XRManager::was_button_pressed(uint8_t button)
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

bool XRManager::was_button_released(uint8_t button)
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

bool XRManager::is_button_touched(uint8_t button)
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

bool XRManager::was_button_touched(uint8_t button)
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

float XRManager::get_trigger_value(eXRHand controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    if (!openxr_context) {
        return 0.0f;
    }
    return openxr_context->triggerValueState[controller].currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    if (!webxr_context) {
        return 0.0f;
    }
    return webxr_context->handButtons[controller][WEBXR_BUTTON_TRIGGER].value;
#else
    return 0.0f;
#endif
}

bool XRManager::is_trigger_pressed(eXRHand controller)
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

bool XRManager::was_trigger_pressed(eXRHand controller)
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

bool XRManager::was_trigger_released(eXRHand controller)
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

bool XRManager::is_trigger_touched(eXRHand controller)
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

bool XRManager::was_trigger_touched(eXRHand controller)
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

bool XRManager::is_grab_pressed(eXRHand controller)
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

bool XRManager::was_grab_pressed(eXRHand controller)
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

bool XRManager::was_grab_released(eXRHand controller)
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

float XRManager::get_grab_value(eXRHand controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    if (!openxr_context) {
        return 0.0f;
    }
    return openxr_context->grabState[controller].currentState;
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    if (!webxr_context) {
        return 0.0f;
    }
    return webxr_context->handButtons[controller][WEBXR_BUTTON_GRAB].value;
#else
    return 0.0f;
#endif
}

/*
 *	Thumbsticks
 */

glm::vec2 XRManager::get_thumbstick_value(eXRHand controller)
{
#if defined(OPENXR_SUPPORT)
    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);
    if (!openxr_context) {
        return { 0.0f, 0.0f };
    }
    return glm::make_vec2(&openxr_context->thumbStickValueState[controller].currentState.x);
#elif defined(WEBXR_SUPPORT)
    WebXRContext* webxr_context = static_cast<WebXRContext*>(xr_context);
    if (!webxr_context) {
        return { 0.0f, 0.0f };
    }
    return webxr_context->axisState[controller];
#else
    return { 0.0f, 0.0f };
#endif
}

bool XRManager::is_thumbstick_pressed(eXRHand controller)
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

bool XRManager::was_thumbstick_pressed(eXRHand controller)
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

bool XRManager::is_thumbstick_touched(eXRHand controller)
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

bool XRManager::was_thumbstick_touched(eXRHand controller)
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

uint8_t XRManager::get_leading_thumbstick_axis(eXRHand controller)
{
#ifdef XR_SUPPORT
    const glm::vec2& value = get_thumbstick_value(controller);
    const glm::vec2& abs_axis = glm::abs(value);
    if (glm::abs(glm::length(abs_axis)) >= xr_thumbstick_deadzone) {
        if (abs_axis.x > abs_axis.y) {
            return XR_THUMBSTICK_AXIS_X;
        } else {
            return XR_THUMBSTICK_AXIS_Y;
        }
    }
#endif
    return XR_THUMBSTICK_NO_AXIS;
}
