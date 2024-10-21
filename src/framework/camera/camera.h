#pragma once

#include "framework/math/math_utils.h"

enum eCameraType {
    CAMERA_ORBIT,
    CAMERA_FLYOVER
};

class Camera {

    enum eCameraProjectionType {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

public:

    Camera() = default;

    virtual void update(float delta_time) {};

    void set_perspective(float fov, float aspect, float z_near, float z_far);
    void set_orthographic(float left, float right, float bottom, float top, float z_near, float z_far);
    virtual void look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals = true);

    void set_view(const glm::mat4x4& view);
    void set_projection(const glm::mat4x4& projection);

    glm::vec3 screen_to_ray(const glm::vec2& mouse_position);

    void update_view_matrix();
    void update_projection_matrix();

    glm::vec3 get_local_vector(const glm::vec3& vector);

    const glm::vec3& get_eye() const { return eye; }
    const glm::vec3& get_center() const { return center; }
    const glm::vec3& get_up() const { return up; }

    void set_center(const glm::vec3& center) { this->center = center; }

    float get_speed() const { return speed; }

    const glm::mat4x4& get_view() const { return view; }
    const glm::mat4x4& get_projection() const { return projection; }
    const glm::mat4x4& get_view_projection() const { return view_projection; }

    void set_speed(float speed) { this->speed = speed; }
    void set_mouse_sensitivity(float mouse_sensitivity) { this->mouse_sensitivity = mouse_sensitivity; }

protected:

    glm::mat4x4 view;
    glm::mat4x4 projection;
    glm::mat4x4 view_projection;

    float left;
    float right;
    float top;
    float bottom;

    eCameraProjectionType type;

    glm::vec3 eye = {};
    glm::vec3 center = {};
    glm::vec3 up = { 0.0f, 1.0f, 0.0f };

    float fov;
    float aspect;
    float z_near;
    float z_far;

    float speed = 1.0f;
    float mouse_sensitivity = 0.01f;
};
