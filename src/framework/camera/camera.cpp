#include "camera.h"
#include "framework/input.h"
#include "graphics/renderer.h"

void Camera::set_perspective(float fov, float aspect, float z_near, float z_far)
{
    type = PERSPECTIVE;

    this->fov = fov;
    this->aspect = aspect;
    this->z_near = z_near;
    this->z_far = z_far;

    update_projection_matrix();
}

void Camera::set_orthographic(float left, float right, float bottom, float top, float z_near, float z_far)
{
    type = ORTHOGRAPHIC;

    this->left = left;
    this->right = right;
    this->bottom = bottom;
    this->top = top;
    this->z_near = z_near;
    this->z_far = z_far;

    update_projection_matrix();
}

const glm::vec3& Camera::screen_to_ray(const glm::vec2& mouse_position)
{
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();
    const glm::mat4x4& view_projection_inv = glm::inverse(get_view_projection());

    glm::vec2 mouse_pos = Input::get_mouse_position();
    glm::vec3 mouse_pos_ndc;
    mouse_pos_ndc.x = (mouse_pos.x / webgpu_context->render_width) * 2.0f - 1.0f;
    mouse_pos_ndc.y = -((mouse_pos.y / webgpu_context->render_height) * 2.0f - 1.0f);
    mouse_pos_ndc.z = 1.0f;

    glm::vec4 ray_dir = view_projection_inv * glm::vec4(mouse_pos_ndc, 1.0f);
    ray_dir /= ray_dir.w;

    return ray_dir;
}

void Camera::look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals)
{
    this->eye = eye;
    this->center = center;
    this->up = up;

    update_view_matrix();
}

void Camera::update_view_matrix()
{
    view = glm::lookAt(eye, center, up);
    view_projection = projection * view;
}

void Camera::update_projection_matrix()
{
    if (type == ORTHOGRAPHIC)
        projection = glm::ortho(left, right, bottom, top, z_near, z_far);
    else
        projection = glm::perspective(fov, aspect, z_near, z_far);

    view_projection = projection * view;
}

glm::vec3 Camera::get_local_vector(const glm::vec3& vector)
{
    glm::mat4x4 inverse_view = glm::inverse(view);
    return inverse_view * glm::vec4(vector, 0.0f);
}
