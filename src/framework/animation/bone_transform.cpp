#include "bone_transform.h"


#define EPSILON 0.0001f
// Transforms can be combined in the same way as matrices and quaternions and the effects of two transforms can be combined into one transform
// To keep things consistent, combining transforms should maintain a right-to-left combination order
Transform combine(const Transform& t1, const Transform& t2) {
	Transform out;
	out.scale = t1.scale * t2.scale;
	out.rotation = t2.rotation * t1.rotation; // right-to-left multiplication (right is the first rotation applyed)
	// The combined position needs to be affected by the rotation and scale
	out.position = t1.rotation * (t1.scale * t2.position); // M = R*S*T
	out.position = t1.position + out.position;
	return out;
}

Transform inverse(const Transform& t) {
	Transform inv;
	inv.rotation = inverse(t.rotation);
	inv.scale.x = fabs(t.scale.x) < EPSILON ? 0.0f : 1.0f / t.scale.x;
	inv.scale.y = fabs(t.scale.y) < EPSILON ? 0.0f : 1.0f / t.scale.y;
	inv.scale.z = fabs(t.scale.z) < EPSILON ? 0.0f : 1.0f / t.scale.z;
	glm::vec3 invTrans = t.position * -1.0f;
	inv.position = inv.rotation * (inv.scale * invTrans);
	return inv;
}

Transform mix(const Transform& a, const Transform& b, float t) {
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
glm::mat4x4 transformToMat4(const Transform& t) {
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

// Extract the rotation and the translition from a matrix is easy. But not for the scale
// M = SRT, ignore the translation: M = SR -> invert R to isolate S
Transform mat4ToTransform(const glm::mat4x4& m) {
	Transform out;
	out.position = glm::vec3(m[3][0], m[3][1], m[3][2]);
	out.rotation = glm::toQuat(m);
	/*glm::mat4x4 rotScaleMat(
		m[0], m[1], m[2], 0.f,
		m[4], m[5], m[6], 0.f,
		m[8], m[9], m[10], 0.f,
		0.f, 0.f, 0.f, 1.f
	);*/
    glm::mat4x4 rotScaleMat(
		m[0][0], m[0][1], m[0][2], 0.f,
		m[1][0], m[1][1], m[1][2], 0.f,
		m[2][0], m[2][1], m[2][2], 0.f,
		0.f, 0.f, 0.f, 1.f
	);
	glm::mat4x4 invRotMat = toMat4(inverse(out.rotation));
	glm::mat4x4 scaleSkewMat = rotScaleMat * invRotMat;
	out.scale = glm::vec3(scaleSkewMat[0][0], scaleSkewMat[1][1], scaleSkewMat[2][2]);
	return out;
}

glm::vec3 transformPoint(const Transform& a, const glm::vec3& b) {
	glm::vec3 out;
	out = a.rotation * (a.scale * b);
	out = a.position + out;
	return out;
}

// First, apply the scale, then rotation
glm::vec3 transformVector(const Transform& t, const glm::vec3& v) {
	glm::vec3 out;
	out = t.rotation * (t.scale * v);
	return out;
}
