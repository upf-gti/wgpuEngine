#include "webxr_context.h"

bool WebXRContext::init(WebGPUContext* webgpu_context)
{
    return false;
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

WGPUTextureView WebXRContext::get_swapchain_view(uint8_t eye_idx)
{
    return WGPUTextureView();
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

