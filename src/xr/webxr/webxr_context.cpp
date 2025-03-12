#include "webxr_context.h"

#include "graphics/webgpu_context.h"

#include <glm/gtc/type_ptr.hpp>

#include "spdlog/spdlog.h"

#include <emscripten.h>
#include <emscripten/html5_webgpu.h>

#ifdef WEBXR_SUPPORT

WebXRContext::~WebXRContext() {
}

bool WebXRContext::init(WebGPUContext* webgpu_context)
{
    spdlog::info("WebXR init");

    webxr_set_device((webgpu_context->get_device()));

    webxr_init(
        /* Frame callback */
        [](void* userData, int time, WebXRRigidTransform* headPose, WebXRView views[2], WGPUTextureView texture_view_left, WGPUTextureView texture_view_right, int viewCount) {
            static_cast<WebXRContext*>(userData)->update_views(views, texture_view_left, texture_view_right);
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

    return true;
}

void WebXRContext::clean()
{
}

bool WebXRContext::begin_session()
{
    return false;
}

bool WebXRContext::end_session()
{
    return false;
}

void WebXRContext::update_views(WebXRView views[2], WGPUTextureView texture_view_left, WGPUTextureView texture_view_right)
{
    for (int i = 0; i < 2; i++) {
        per_view_data[i].position = glm::make_vec3(views[i].viewPose.position);
        per_view_data[i].projection_matrix = glm::make_mat4(views[i].projectionMatrix);
        per_view_data[i].view_matrix = glm::make_mat4(views[i].viewPose.matrix);
        per_view_data[i].view_projection_matrix = per_view_data[i].projection_matrix * per_view_data[i].view_matrix;
    }

    viewport = glm::make_vec4(views[0].viewport);
      
    swapchain_views[0] = texture_view_left;
    swapchain_views[1] = texture_view_right;
}

WGPUTextureView WebXRContext::get_swapchain_view(uint8_t eye_idx)
{
    return swapchain_views[eye_idx];
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
    default:
        spdlog::error("Unknown WebXR error with code: {}", error);
    }
}

#endif
