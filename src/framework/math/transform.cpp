#include "transform.h"

#define EPSILON 0.000001f

#include "glm/gtx/compatibility.hpp"
#include "glm/gtx/norm.hpp"


Transform::Transform(const glm::vec3& p, const glm::quat& r, const glm::vec3& s) :
    position_p(p), rotation_p(r), scale_p(s)
{
    model = Transform::transform_to_mat4(*this);
}

Transform::Transform() :
    position_p(glm::vec3(0.0f)),
    rotation_p(glm::quat(0.0f, 0.0f, 0.0f, 1.0f)),
    scale_p(glm::vec3(1.0f))
{
    model = glm::identity<glm::mat4x4>();
}

void Transform::set_position(const glm::vec3& position)
{
    this->position_p = position;
    dirty = true;
}

void Transform::set_rotation(const glm::quat& rotation)
{
    this->rotation_p = rotation;
    dirty = true;
}

void Transform::set_scale(const glm::vec3& scale)
{
    this->scale_p = scale;
    dirty = true;
}

void Transform::translate(const glm::vec3& translation)
{
    position_p += translation;
    dirty = true;
}

void Transform::rotate(const glm::quat& rotation_factor)
{
    rotation_p = rotation_p * rotation_factor;
    dirty = true;
}

void Transform::scale(const glm::vec3& scale_factor)
{
    scale_p = scale_p * scale_factor;
    dirty = true;
}

void Transform::rotate_world(const glm::quat& rotation_factor)
{
    rotate(glm::inverse(rotation_p) * rotation_factor * rotation_p);
}

const glm::vec3& Transform::get_position() const
{
    return position_p;
}

const glm::quat& Transform::get_rotation() const
{
    return rotation_p;
}

const glm::vec3& Transform::get_scale() const
{
    return scale_p;
}

glm::vec3& Transform::get_position_ref()
{
    return position_p;
}

glm::quat& Transform::get_rotation_ref()
{
    return rotation_p;
}

glm::vec3& Transform::get_scale_ref()
{
    return scale_p;
}

void Transform::set_dirty(bool dirty)
{
    this->dirty = dirty;
}

bool Transform::is_dirty()
{
    return dirty;
}

const glm::mat4x4& Transform::get_model()
{
    if (dirty) {
        model = Transform::transform_to_mat4(*this);
        dirty = false;
    }

    return model;
}

Transform Transform::identity()
{
    Transform out;
    out.position_p = { 0.0f, 0.0f, 0.0f };
    out.rotation_p = { 0.0f, 0.0f, 0.0f, 1.0f };
    out.scale_p = { 1.0f, 1.0f, 1.0f };
    return out;
}

// Transforms can be combined in the same way as matrices and quaternions and the effects of two transforms can be combined into one transform
// To keep things consistent, combining transforms should maintain a right-to-left combination order
Transform Transform::combine(const Transform& t1, const Transform& t2)
{
    Transform out;
    out.scale_p = t1.scale_p * t2.scale_p;
    out.rotation_p = t1.rotation_p * t2.rotation_p;
    // The combined position needs to be affected by the rotation and scale
    out.position_p = t1.rotation_p * (t1.scale_p * t2.position_p); // M = T*R*S
    out.position_p = t1.position_p + out.position_p;
    return out;
}

Transform Transform::inverse(const Transform& t)
{
    Transform inv;
    inv.rotation_p = glm::inverse(t.rotation_p);
    inv.scale_p.x = fabs(t.scale_p.x) < EPSILON ? 0.0f : 1.0f / t.scale_p.x;
    inv.scale_p.y = fabs(t.scale_p.y) < EPSILON ? 0.0f : 1.0f / t.scale_p.y;
    inv.scale_p.z = fabs(t.scale_p.z) < EPSILON ? 0.0f : 1.0f / t.scale_p.z;
    glm::vec3 invTrans = t.position_p * -1.0f;
    inv.position_p = inv.rotation_p * (inv.scale_p * invTrans);
    return inv;
}

Transform Transform::mix(const Transform& a, const Transform& b, float t)
{
    glm::quat bRot = b.rotation_p;
    if (dot(a.rotation_p, bRot) < 0.0f) {
        bRot = -bRot;
    }
    return Transform(
        lerp(a.position_p, b.position_p, t),
        slerp(a.rotation_p, bRot, t), //originally "nlerp"
        lerp(a.scale_p, b.scale_p, t));
}

// Converts a transform into a mat4
glm::mat4x4 Transform::transform_to_mat4(const Transform& t)
{
    // First, get the rotation basis of the transform
    glm::vec3 x = t.rotation_p * glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 y = t.rotation_p * glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 z = t.rotation_p * glm::vec3(0.f, 0.f, 1.f);
    // Next, scale the basis vectors
    x = x * t.scale_p.x;
    y = y * t.scale_p.y;
    z = z * t.scale_p.z;
    // Get the position of the transform
    glm::vec3 p = t.position_p;
    // Create matrix
    return glm::mat4x4(
        x.x, x.y, x.z, 0.f, // X basis (& Scale)
        y.x, y.y, y.z, 0.f, // Y basis (& scale)
        z.x, z.y, z.z, 0.f, // Z basis (& scale)
        p.x, p.y, p.z, 1.f  // Position
    );
}

bool is_equal(const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 diff(a - b);
    return glm::length2(diff) < EPSILON;
}

glm::vec3 normalized(const glm::vec3& v) {
    float len_squared = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len_squared < EPSILON)
        return v;
    return normalize(v);
}

glm::quat Transform::get_rotation_between_vectors(const glm::vec3& from, const glm::vec3& to)
{
    glm::vec3 f = normalized(from);
    glm::vec3 t = normalized(to);

    if (is_equal(f,t)) { // parallel vectors 
        return glm::quat(0.f, 0.f, 0.f, 1.f);
    }
    else if (is_equal(f, t * -1.0f)) { 	// check whether the two vectors are opposites of each other
        glm::vec3 ortho = glm::vec3(1.f, 0.f, 0.f);
        if (fabsf(f.y) < fabsf(f.x)) {
            ortho = glm::vec3(0.f, 1.f, 0.f);
        }
        if (fabsf(f.z) < fabs(f.y) && fabs(f.z) < fabsf(f.x)) {
            ortho = glm::vec3(0.f, 0.f, 1.f);
        }
        glm::vec3 axis = normalized(cross(f, ortho));
        return glm::quat(axis.x, axis.y, axis.z, 0.f);
    }

    // half vector between the from and to vectors
    glm::vec3 half = normalized(f + t);
    glm::vec3 axis = cross(f, half);
    return glm::quat(axis.x, axis.y, axis.z, dot(f, half));
}

glm::quat look_rotation(const glm::vec3& direction, const glm::vec3& up)
{
    // Find orthonormal basis vectors
    glm::vec3 f = normalized(direction); // Object Forward
    glm::vec3 u = normalized(up); // Desired Up
    glm::vec3 r = cross(u, f); // Object Right
    u = cross(f, r); // Object Up
    // From world forward to object forward
    glm::quat worldToObject = Transform::get_rotation_between_vectors(glm::vec3(0.f, 0.f, 1.f), f);
    // what direction is the new object up?
    glm::vec3 objectUp = worldToObject * glm::vec3(0.f, 1.f, 0.f);
    // From object up to desired up
    glm::quat u2u = Transform::get_rotation_between_vectors(objectUp, u);
    // Rotate to forward direction first
    // then twist to correct up
    glm::quat result = u2u * worldToObject;
    // Don't forget to normalize the result
    return normalize(result);
}

// Extract the rotation and the translition from a matrix is easy. But not for the scale
// M = SRT, ignore the translation: M = SR -> invert R to isolate S
Transform Transform::mat4_to_transform(const glm::mat4x4& m)
{
    Transform out;

    out.position_p = glm::vec3(m[3]);
    glm::vec3 up = normalized(glm::vec3(m[1]));
    glm::vec3 forward = normalized(glm::vec3(m[2]));
    glm::vec3 right = cross(up, forward);
    up = cross(forward, right);

    out.rotation_p = look_rotation(forward, up);//glm::toQuat(m);
    glm::mat4x4 rotScaleMat = m;
    rotScaleMat[0][3] = 0.f;
    rotScaleMat[1][3] = 0.f;
    rotScaleMat[2][3] = 0.f;
    rotScaleMat[3][3] = 1.f;

    glm::quat inv_rot = glm::inverse(out.rotation_p);
    glm::vec3 r = inv_rot * glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 u = inv_rot * glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 f = inv_rot * glm::vec3(0.f, 0.f, 1.f);
    glm::mat4x4 invRotMat =
        glm::mat4x4(r.x, r.y, r.z, 0.f,
            u.x, u.y, u.z, 0.f,
            f.x, f.y, f.z, 0.f,
            0.f, 0.f, 0.f, 1.f
        );
    glm::mat4x4 scaleSkewMat = invRotMat * rotScaleMat;
    out.scale_p = glm::vec3(scaleSkewMat[0][0], scaleSkewMat[1][1], scaleSkewMat[2][2]);
    return out;
}

glm::vec3 Transform::transform_point(const Transform& a, const glm::vec3& b)
{
    glm::vec3 out;
    out = a.rotation_p * (a.scale_p * b);
    out = a.position_p + out;
    return out;
}

// First, apply the scale, then rotation
glm::vec3 Transform::transform_vector(const Transform& t, const glm::vec3& v)
{
    glm::vec3 out;
    out = t.rotation_p * (t.scale_p * v);
    return out;
}

// In radians
float Transform::get_angle_between_vectors(const glm::vec3& l, const glm::vec3& r) {
    float sqMagL = l.x * l.x + l.y * l.y + l.z * l.z;
    float sqMagR = r.x * r.x + r.y * r.y + r.z * r.z;
    if (sqMagL < EPSILON || sqMagR < EPSILON) {
        return 0.0f;
    }
    float dot = l.x * r.x + l.y * r.y + l.z * r.z;
    float len = sqrtf(sqMagL) * sqrtf(sqMagR);
    return acosf(dot / len); //rad
}
