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

uint32_t XRContext::get_num_images_per_swapchain()
{
    return 2;
}

glm::mat4x4 XrInputPose_to_glm(const XrInputPose& p) {
    glm::mat4 translation = glm::translate(glm::mat4{ 1.f }, glm::vec3(p.position.x, p.position.y, p.position.z));
    glm::mat4 orientation = glm::mat4_cast(glm::quat(p.orientation.x, p.orientation.y, p.orientation.z, p.orientation.w));
    return translation * orientation;
}
