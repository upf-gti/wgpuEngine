#include "bone_transform.h"

#define EPSILON 0.0001f

#include "glm/gtx/compatibility.hpp"

// Transforms can be combined in the same way as matrices and quaternions and the effects of two transforms can be combined into one transform
// To keep things consistent, combining transforms should maintain a right-to-left combination order
Transform combine(const Transform& t1, const Transform& t2)
{
    Transform out;
    out.scale = t1.scale * t2.scale;
    out.rotation = t1.rotation * t2.rotation;
    // The combined position needs to be affected by the rotation and scale
    out.position = t1.rotation * (t1.scale * t2.position); // M = T*R*S
    out.position = t1.position + out.position;
    return out;
}

Transform inverse(const Transform& t)
{
    Transform inv;
    inv.rotation = inverse(t.rotation);
    inv.scale.x = fabs(t.scale.x) < EPSILON ? 0.0f : 1.0f / t.scale.x;
    inv.scale.y = fabs(t.scale.y) < EPSILON ? 0.0f : 1.0f / t.scale.y;
    inv.scale.z = fabs(t.scale.z) < EPSILON ? 0.0f : 1.0f / t.scale.z;
    glm::vec3 invTrans = t.position * -1.0f;
    inv.position = inv.rotation * (inv.scale * invTrans);
    return inv;
}

Transform mix(const Transform& a, const Transform& b, float t)
{
    glm::quat bRot = b.rotation;
    if (dot(a.rotation, bRot) < 0.0f) {
        bRot = -bRot;
    }
    return Transform(
        lerp(a.position, b.position, t),
        slerp(a.rotation, bRot, t), //originally "nlerp"
        lerp(a.scale, b.scale, t));
}

// Converts a transform into a mat4
glm::mat4x4 transformToMat4(const Transform& t)
{
    // First, get the rotation basis of the transform
    glm::vec3 x = t.rotation * glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 y = t.rotation * glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 z = t.rotation * glm::vec3(0.f, 0.f, 1.f);
    // Next, scale the basis vectors
    x = x * t.scale.x;
    y = y * t.scale.y;
    z = z * t.scale.z;
    // Get the position of the transform
    glm::vec3 p = t.position;
    // Create matrix
    return glm::mat4x4(
        x.x, x.y, x.z, 0.f, // X basis (& Scale)
        y.x, y.y, y.z, 0.f, // Y basis (& scale)
        z.x, z.y, z.z, 0.f, // Z basis (& scale)
        p.x, p.y, p.z, 1.f  // Position
    );
}

glm::quat fromTo(const glm::vec3& from, const glm::vec3& to)
{
    glm::vec3 f = normalize(from);
    glm::vec3 t = normalize(to);

    if (f == t) { // parallel vectors 
        return glm::quat(0.f, 0.f, 0.f, 1.f);
    }
    else if (f == t * -1.0f) { 	// check whether the two vectors are opposites of each other
        glm::vec3 ortho = glm::vec3(1, 0, 0);
        if (fabsf(f.y) < fabsf(f.x)) {
            ortho = glm::vec3(0, 1, 0);
        }
        if (fabsf(f.z) < fabs(f.y) && fabs(f.z) < fabsf(f.x)) {
            ortho = glm::vec3(0, 0, 1);
        }
        glm::vec3 axis = normalize(cross(f, ortho));
        return glm::quat(axis.x, axis.y, axis.z, 0);
    }
    // half vector between the from and to vectors
    glm::vec3 half = normalize(f + t);
    glm::vec3 axis = cross(f, half);
    return glm::quat(axis.x, axis.y, axis.z, dot(f, half));
}

glm::quat lookRotation(const glm::vec3& direction, const glm::vec3& up)
{
    // Find orthonormal basis vectors
    glm::vec3 f = normalize(direction); // Object Forward
    glm::vec3 u = normalize(up); // Desired Up
    glm::vec3 r = cross(u, f); // Object Right
    u = cross(f, r); // Object Up
    // From world forward to object forward
    glm::quat worldToObject = fromTo(glm::vec3(0, 0, 1), f);
    // what direction is the new object up?
    glm::vec3 objectUp = worldToObject * glm::vec3(0, 1, 0);
    // From object up to desired up
    glm::quat u2u = fromTo(objectUp, u);
    // Rotate to forward direction first
    // then twist to correct up
    glm::quat result = u2u * worldToObject;
    // Don't forget to normalize the result
    return normalize(result);
}

// Extract the rotation and the translition from a matrix is easy. But not for the scale
// M = SRT, ignore the translation: M = SR -> invert R to isolate S
Transform mat4ToTransform(const glm::mat4x4& m)
{
    Transform out;

    out.position = glm::vec3(m[3]);
    glm::vec3 up = normalize(glm::vec3(m[1]));
    glm::vec3 forward = normalize(glm::vec3(m[2]));
    glm::vec3 right = cross(up, forward);
    up = cross(forward, right);

    out.rotation = lookRotation(forward, up);//glm::toQuat(m);
    glm::mat4x4 rotScaleMat = m;
    rotScaleMat[0][3] = 0.f;
    rotScaleMat[1][3] = 0.f;
    rotScaleMat[2][3] = 0.f;
    rotScaleMat[3][3] = 1.f;

    glm::quat inv_rot = inverse(out.rotation);
    glm::vec3 r = inv_rot * glm::vec3(1, 0, 0);
    glm::vec3 u = inv_rot * glm::vec3(0, 1, 0);
    glm::vec3 f = inv_rot * glm::vec3(0, 0, 1);
    glm::mat4x4 invRotMat =
        glm::mat4x4(r.x, r.y, r.z, 0,
            u.x, u.y, u.z, 0,
            f.x, f.y, f.z, 0,
            0, 0, 0, 1
        );
    glm::mat4x4 scaleSkewMat = rotScaleMat * invRotMat;
    out.scale = glm::vec3(scaleSkewMat[0][0], scaleSkewMat[1][1], scaleSkewMat[2][2]);
    return out;
}

glm::vec3 transformPoint(const Transform& a, const glm::vec3& b)
{
    glm::vec3 out;
    out = a.rotation * (a.scale * b);
    out = a.position + out;
    return out;
}

// First, apply the scale, then rotation
glm::vec3 transformVector(const Transform& t, const glm::vec3& v)
{
    glm::vec3 out;
    out = t.rotation * (t.scale * v);
    return out;
}
