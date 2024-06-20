#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/quaternion.hpp"

class Transform {

    // internal properties
	glm::vec3 position_p;
	glm::quat rotation_p;
	glm::vec3 scale_p;

    glm::mat4x4 model;

    bool dirty = false;

public:

    Transform(const glm::vec3& p, const glm::quat& r, const glm::vec3& s);
    Transform();

    void set_position(const glm::vec3& position);
    void set_rotation(const glm::quat& rotation);
    void set_scale(const glm::vec3& scale);

    const glm::vec3& get_position() const;
    const glm::quat& get_rotation() const;
    const glm::vec3& get_scale() const;

    glm::vec3& get_position_ref();
    glm::quat& get_rotation_ref();
    glm::vec3& get_scale_ref();

    void set_dirty(bool dirty);

    // true if it changed during this frame
    bool is_dirty();

    void translate(const glm::vec3& translation);
    void rotate(const glm::quat& rotation_factor);
    void scale(const glm::vec3& scale_factor);

    const glm::mat4x4& get_model();

    static Transform identity();
    static Transform combine(const Transform& a, const Transform& b);
    static Transform inverse(const Transform& t);
    static Transform mix(const Transform& a, const Transform& b, float t);
    static glm::mat4x4 transform_to_mat4(const Transform& t);
    static Transform mat4_to_transform(const glm::mat4x4& m);
    static glm::vec3 transform_point(const Transform& a, const glm::vec3& b);
    static glm::vec3 transform_vector(const Transform& a, const glm::vec3& b);
    static glm::quat get_rotation_between_vectors(const glm::vec3& from, const glm::vec3& to);
    static float get_angle_between_vectors(const glm::vec3& l, const glm::vec3& r);

};
