#pragma once

#include "camera_3d.h"

class EditorCamera : public Camera3D {
public:
    EditorCamera();

    void update(float delta_time) override;

    void look_at(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool reset_internals = true) override;

    LerpedValue<float> distance_lerp;
    LerpedValue<glm::vec3> move_dir_lerp;

    float distance = 0.0f;

private:
    void custom_update(float delta_time);
    void check_update_mode();

    bool flyover_enabled = false;
    bool orbit_enabled = false;
    bool pan_enabled = false;
};
