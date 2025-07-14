#pragma once

#include "framework/math/math_utils.h"

enum eCameraType {
    CAMERA_ORBIT,
    CAMERA_FLYOVER
};

class Camera {
public:

    enum eCameraProjectionType {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

    Camera() = default;

    virtual void update(float delta_time) {};

    void update_view_matrix();
    void update_projection_matrix();
    void update_view_projection_matrix();

    virtual void look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals = true);

    glm::vec3 screen_to_ray(const glm::vec2& mouse_position);

    void set_perspective(float fov, float aspect, float z_near, float z_far);
    void set_orthographic(float left, float right, float bottom, float top, float z_near, float z_far);

    void set_view(const glm::mat4x4& view, bool update_view_projection = true);
    void set_projection(const glm::mat4x4& projection, bool update_view_projection = true);
    void set_view_projection(const glm::mat4x4& view_projection);

    void set_eye(const glm::vec3& new_eye);
    void set_center(const glm::vec3& new_center);
    void set_up(const glm::vec3& new_up);

    void set_speed(float speed) { this->speed = speed; }
    void set_mouse_sensitivity(float mouse_sensitivity) { this->mouse_sensitivity = mouse_sensitivity; }

    glm::vec3 get_local_vector(const glm::vec3& vector);

    const glm::vec3& get_eye() const { return eye; }
    const glm::vec3& get_center() const { return center; }
    const glm::vec3& get_up() const { return up; }

    float get_fov() const { return fov; }
    float get_aspect() const { return aspect; }
    float get_near() const { return z_near; }
    float get_far() const { return z_far; }
    float get_left() const { return left; }
    float get_right() const { return right; }
    float get_top() const { return top; }
    float get_bottom() const { return bottom; }
    float get_speed() const { return speed; }
    float get_mouse_sensitivity() const { return mouse_sensitivity; }

    const glm::mat4x4& get_view() const { return view; }
    const glm::mat4x4& get_projection() const { return projection; }
    const glm::mat4x4& get_view_projection() const { return view_projection; }

protected:

    glm::mat4x4 view;
    glm::mat4x4 projection;
    glm::mat4x4 view_projection;

    float left;
    float right;
    float top;
    float bottom;

    eCameraProjectionType type;

    float fov;
    float aspect;
    float z_near;
    float z_far;

    float speed = 1.0f;
    float mouse_sensitivity = 0.01f;

#ifdef __EMSCRIPTEN__
public:
#endif

    glm::vec3 eye = {};
    glm::vec3 center = {};
    glm::vec3 up = { 0.0f, 1.0f, 0.0f };
};
