#include "webxr_context.h"

#include "engine/engine.h"

#include "graphics/webgpu_context.h"

#include "framework/input.h"

#include "spdlog/spdlog.h"

#include <glm/gtc/type_ptr.hpp>

#include <emscripten.h>
#include <emscripten/html5_webgpu.h>

#ifdef WEBXR_SUPPORT

XrInputPose parse_WebXR_pose_to_XrInputPose(const WebXRRigidTransform& p);

WebXRContext::~WebXRContext()
{

}

bool WebXRContext::init(WebGPUContext* webgpu_context)
{
    spdlog::info("WebXR init");

    webxr_set_device((webgpu_context->get_device()));

    webxr_init(
        /* Frame callback */
        [](void* userData, int time, WebXRRigidTransform* head_pose, WebXRView views[2], WGPUTextureView texture_view_left, WGPUTextureView texture_view_right, int viewCount) {
            static_cast<WebXRContext*>(userData)->on_frame(head_pose, views, texture_view_left, texture_view_right);
        },
        /* Session WebXR init callback */
        [](void* userData) {
            //webxr_request_session(WEBXR_SESSION_MODE_IMMERSIVE_VR, WEBXR_SESSION_FEATURE_WEBGPU);
        },
        /* Session begin callback */
        [](void* userData, int mode) {
            static_cast<WebXRContext*>(userData)->begin_session();
        },
        /* Session end callback */
        [](void* userData, int mode) {
            static_cast<WebXRContext*>(userData)->end_session();
        },
        /* Error callback */
        [](void* userData, int error) {
            static_cast<WebXRContext*>(userData)->print_error(error);
        },
        /* userData */
        this
    );

    if (Input::init_xr(this)) {
        spdlog::error("Can't initialize WebXR input");
        return 1;
    }

    return true;
}

void WebXRContext::clean()
{
}

void WebXRContext::on_frame(WebXRRigidTransform* head_pose, WebXRView views[2], WGPUTextureView texture_view_left, WGPUTextureView texture_view_right)
{
    update_views(head_pose, views, texture_view_left, texture_view_right);

    poll_actions();

    Engine::instance->on_frame(); // Make frame from here to synchronise render and xr session
}

bool WebXRContext::begin_session()
{
    emscripten_pause_main_loop();

    return false;
}

bool WebXRContext::end_session()
{
    emscripten_resume_main_loop();

    return false;
}

void WebXRContext::update_views(WebXRRigidTransform* head_pose, WebXRView views[2], WGPUTextureView texture_view_left, WGPUTextureView texture_view_right)
{
    for (int i = 0; i < EYE_COUNT; i++) {
        per_view_data[i].position = glm::make_vec3(views[i].viewPose.position);
        per_view_data[i].projection_matrix = glm::make_mat4(views[i].projectionMatrix);
        per_view_data[i].view_matrix = glm::make_mat4(views[i].viewPose.matrix);
        per_view_data[i].view_projection_matrix = per_view_data[i].projection_matrix * per_view_data[i].view_matrix;
    }

    viewport = glm::make_vec4(views[0].viewport);
      
    swapchain_views[EYE_LEFT] = texture_view_left;
    swapchain_views[EYE_RIGHT] = texture_view_right;

    // Head pose
    {
        headPose = parse_WebXR_pose_to_XrInputPose(*head_pose);
        headPoseMatrix = XrInputPose_to_glm(headPose);
    }
}

WGPUTextureView WebXRContext::get_swapchain_view(uint8_t eye_idx)
{
    return swapchain_views[eye_idx];
}

void WebXRContext::poll_actions()
{
    WebXRInputSource sources[HAND_COUNT];
    int sources_count = 0;
    webxr_get_input_sources(sources, 5, &sources_count);

    WebXRRigidTransform webxr_transform;

    for(int s = 0; s < sources_count; ++s) {

        WebXRInputSource* source = &sources[s];

        // Poses
        webxr_get_input_pose(source, &webxr_transform, WEBXR_INPUT_POSE_GRIP);
        controllerGripPoses[s] = parse_WebXR_pose_to_XrInputPose(webxr_transform);
        controllerGripPoseMatrices[s] = XrInputPose_to_glm(controllerGripPoses[s]);

        webxr_get_input_pose(source, &webxr_transform, WEBXR_INPUT_POSE_TARGET_RAY);
        controllerAimPoses[s] = parse_WebXR_pose_to_XrInputPose(webxr_transform);
        controllerAimPoseMatrices[s] = XrInputPose_to_glm(controllerAimPoses[s]);

        // Buttons
        if(handButtons[s].size() != sources_count) {
            handButtons[s].resize(sources_count);
        }

        // TODO: Refactor this to get all buttons at once and avoid making different emscripten calls
        for (int b = 0; b < WEBXR_BUTTON_COUNT; ++b) {
            GamepadButton button;
            webxr_get_input_button(source, b, &button);
            // Update changedSinceLastSync to get was pressed/release events
            {
                button.changedSinceLastSync[GAMEPAD_BUTTON_PRESSED_STATE] = (button.pressed != handButtons[s][b].pressed);
                button.changedSinceLastSync[GAMEPAD_BUTTON_TOUCHED_STATE] = (button.touched != handButtons[s][b].touched);
                button.changedSinceLastSync[GAMEPAD_BUTTON_VALUE_STATE] = (button.value != handButtons[s][b].value);
            }
            handButtons[s][b] = button;
        }

        webxr_get_input_axes(source, &axisState[s].x);
    }

    // map buttons to XR_BUTTONS
    buttonsState[XR_BUTTON_A] = handButtons[HAND_RIGHT][WEBXR_BUTTON_AX];
    buttonsState[XR_BUTTON_B] = handButtons[HAND_RIGHT][WEBXR_BUTTON_BY];
    buttonsState[XR_BUTTON_X] = handButtons[HAND_LEFT][WEBXR_BUTTON_AX];
    buttonsState[XR_BUTTON_Y] = handButtons[HAND_LEFT][WEBXR_BUTTON_BY];
    buttonsState[XR_BUTTON_MENU] = {}; // not used in webxr
}

void WebXRContext::update()
{

}

void WebXRContext::print_viewconfig_view_info()
{
}

void WebXRContext::print_reference_spaces()
{
}

void WebXRContext::print_error(int error)
{
    switch (error) {
    case WEBXR_ERR_WEBXR_UNSUPPORTED:
        spdlog::error("WebXR unsupported in this browser");
        break;
    case WEBXR_ERR_WEBGPU_UNSUPPORTED:
        spdlog::error("WebGPU unsupported in this browser");
        break;
    case WEBXR_ERR_XRGPU_BINDING_UNSUPPORTED:
        spdlog::error("WebXR/WebGPU binding not supported on this device");
        break;
    case WEBXR_ERR_IMMERSIVE_XR_UNSUPPORTED:
        spdlog::error("Immersive VR not supported. Missing Headset?");
        break;
    default:
        spdlog::error("Unknown WebXR error with code: {}", error);
    }
}

inline XrInputPose parse_WebXR_pose_to_XrInputPose(const WebXRRigidTransform& p) {
    return { glm::quat(p.orientation[0], p.orientation[1], p.orientation[2], p.orientation[3]), glm::vec3(p.position[0], p.position[1], p.position[2]) };
}

#endif
