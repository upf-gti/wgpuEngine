#include "xr_context.h"

XRContext::~XRContext()
{
}

uint32_t XRContext::get_swapchain_image_index(uint8_t eye_idx)
{
    return 0;
}

void XRContext::init_frame()
{
}

void XRContext::acquire_swapchain(int swapchain_index)
{
}

void XRContext::release_swapchain(int swapchain_index)
{
}

void XRContext::end_frame()
{
}
