#include "camera.h"

#include "framework/input.h"
#include "framework/entities/entity_mesh.h"

#include "framework/utils/utils.h"

void Camera::update(float delta_time)
{

    if (Input::is_mouse_pressed(GLFW_MOUSE_BUTTON_LEFT))
    {

        {
            glm::vec2 mouse_delta = Input::get_mouse_delta();

            mouse_delta.x = glm::clamp(mouse_delta.x, -150.0f, 150.0f);

            delta_yaw += mouse_delta.x * mouse_sensitivity;
            delta_pitch -= mouse_delta.y * mouse_sensitivity;
        }

        delta_yaw = clamp_rotation(delta_yaw);
        delta_pitch = clamp_rotation(delta_pitch);

        float max_offset = 0.25f;

        if (delta_pitch >= pi_2 - max_offset && delta_pitch < pi) {
            delta_pitch = pi_2 - 0.001f - max_offset;
        }

        if (delta_pitch > pi && delta_pitch <= 3.0f * pi_2 + max_offset) {
            delta_pitch = 3.0f * pi_2 + 0.001f + max_offset;
        }
    }

    delta_pitch_lerp.value = smooth_damp_angle(delta_pitch_lerp.value, delta_pitch, &delta_pitch_lerp.velocity, 0.05f, 40.0f, delta_time);
    delta_yaw_lerp.value = smooth_damp_angle(delta_yaw_lerp.value, delta_yaw, &delta_yaw_lerp.velocity, 0.05f, 40.0f, delta_time);
}

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

void Camera::look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals)
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

void Camera::look_at_entity(Entity* entity)
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
