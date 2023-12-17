#pragma once

#include "glm/glm.hpp"

class Entity;

class Camera {

    enum eCameraType {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

public:

    Camera() = default;

    virtual void update(float delta_time);

    void set_perspective(float fov, float aspect, float z_near, float z_far);
    void set_orthographic(float left, float right, float bottom, float top, float z_near, float z_far);
    void look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

    void look_at_entity(Entity* entity);

    void update_view_matrix();
    void update_projection_matrix();

    glm::vec3 get_local_vector(const glm::vec3& vector);

    const glm::vec3& get_eye() const { return eye; }
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

    float delta_yaw = 0.0f;
    float delta_pitch = 0.0f;

    eCameraType type;

    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up;

    float fov;
    float aspect;
    float z_near;
    float z_far;

    float speed = 1.0f;
    float mouse_sensitivity = 1.0f;
};