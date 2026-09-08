#include "camera_3d.h"

#include "core/managers/input/input_manager.h"
#include "core/managers/render/render_manager.h"

void Camera3D::set_perspective(float fov, float aspect, float z_near, float z_far)
{
    type = PERSPECTIVE;

    this->fov = fov;
    this->aspect = aspect;
    this->z_near = z_near;
    this->z_far = z_far;

    update_projection_matrix();
}

void Camera3D::set_orthographic(float left, float right, float bottom, float top, float z_near, float z_far)
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

void Camera3D::update(float delta_time)
{
    if (InputManager::get_singleton()->is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT)) {
        apply_movement(InputManager::get_singleton()->get_mouse_delta());
    }

    delta_pitch_lerp.value = smooth_damp_angle(delta_pitch_lerp.value, delta_pitch, &delta_pitch_lerp.velocity, 0.05f, 40.0f, delta_time);
    delta_yaw_lerp.value = smooth_damp_angle(delta_yaw_lerp.value, delta_yaw, &delta_yaw_lerp.velocity, 0.05f, 40.0f, delta_time);
}

glm::vec3 Camera3D::screen_to_ray(const glm::vec2& mouse_position)
{
    const glm::mat4x4& view_projection_inv = glm::inverse(get_view_projection());

    glm::vec2 mouse_pos = InputManager::get_singleton()->get_mouse_position();
    glm::vec3 mouse_pos_ndc;
    mouse_pos_ndc.x = (mouse_pos.x / RenderManager::get_singleton()->get_render_width()) * 2.0f - 1.0f;
    mouse_pos_ndc.y = -((mouse_pos.y / RenderManager::get_singleton()->get_render_height()) * 2.0f - 1.0f);
    mouse_pos_ndc.z = 0.0f;

    glm::vec4 ray_dir = view_projection_inv * glm::vec4(mouse_pos_ndc, 1.0f);
    ray_dir /= ray_dir.w;

    return ray_dir;
}

void Camera3D::look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals)
{
    this->eye = eye;
    this->center = center;
    this->up = up;

    update_view_matrix();

    if (reset_internals) {
        vector_to_yaw_pitch(glm::normalize(glm::vec3(center - eye)), &delta_yaw, &delta_pitch);
        delta_yaw_lerp.value = delta_yaw;
        delta_pitch_lerp.value = delta_pitch;
        eye_lerp.value = eye;
    }
}

void Camera3D::set_view(const glm::mat4x4& view, bool update_view_projection)
{
    this->view = view;

    // TODO: make if constexpr?
    if (update_view_projection) {
        view_projection = projection * view;
    }
}

void Camera3D::set_projection(const glm::mat4x4& projection, bool update_view_projection)
{
    this->projection = projection;

    // TODO: make if constexpr?
    if (update_view_projection) {
        view_projection = projection * view;
    }
}

void Camera3D::set_view_projection(const glm::mat4x4& view_projection)
{
    this->view_projection = view_projection;
}

void Camera3D::set_eye(const glm::vec3& new_eye)
{
    eye = new_eye;

    update_view_matrix();
}

void Camera3D::set_center(const glm::vec3& new_center)
{
    center = new_center;

    update_view_matrix();
}

void Camera3D::set_up(const glm::vec3& new_up)
{
    up = new_up;

    update_view_matrix();
}

void Camera3D::update_view_matrix()
{
    view = glm::lookAt(eye, center, up);
    view_projection = projection * view;
}

void Camera3D::update_projection_matrix()
{
    // z_near and z_far are inverted for reverse z-buffer
    if (type == ORTHOGRAPHIC) {
        projection = glm::ortho(left, right, bottom, top, z_far, z_near);
    } else {
        projection = glm::perspective(fov, aspect, z_far, z_near);
    }

    view_projection = projection * view;
}

void Camera3D::update_view_projection_matrix()
{
    view_projection = projection * view;
}

glm::vec3 Camera3D::get_local_vector(const glm::vec3& vector)
{
    glm::mat4x4 inverse_view = glm::inverse(view);
    return inverse_view * glm::vec4(vector, 0.0f);
}

void Camera3D::look_at_node(Node3D* entity)
{
    if (!entity) {
        return;
    }

    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);

    AABB aabb = entity->get_aabb();

    float distance = 0.5f;

    if (aabb.initialized()) {
        distance = 3.0f * std::max(std::max(abs(aabb.half_size.x), abs(aabb.half_size.y)), abs(aabb.half_size.z));
    }

    look_at(aabb.center - distance * front, aabb.center, glm::vec3(0.0, 1.0, 0.0));
}

void Camera3D::apply_movement(const glm::vec2& movement)
{
    {
        glm::vec2 delta = movement;

        delta.x = glm::clamp(delta.x, -150.0f, 150.0f);

        delta_yaw += delta.x * mouse_sensitivity;
        delta_pitch -= delta.y * mouse_sensitivity;
    }

    delta_yaw = clamp_rotation(delta_yaw);
    delta_pitch = clamp_rotation(delta_pitch);

    float max_offset = 0.25f;

    if (delta_pitch >= PI_2_F - max_offset && delta_pitch < PI) {
        delta_pitch = PI_2_F - 0.001f - max_offset;
    }

    if (delta_pitch > PI_F && delta_pitch <= 3.0f * PI_2_F + max_offset) {
        delta_pitch = 3.0f * PI_2_F + 0.001f + max_offset;
    }
}
