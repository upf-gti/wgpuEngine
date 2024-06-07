#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/quaternion.hpp"

struct Transform {

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	Transform(const glm::vec3& p, const glm::quat& r, const glm::vec3& s) :
		position(p), rotation(r), scale(s) {}

	Transform() :
		position(glm::vec3(0.0f)),
		rotation(glm::quat(0.0f, 0.0f, 0.0f, 1.0f)),
		scale(glm::vec3(1.0f)) {}
};

Transform combine(const Transform& a, const Transform& b);
Transform inverse(const Transform& t);
Transform mix(const Transform& a, const Transform& b, float t);
glm::mat4x4 transformToMat4(const Transform& t);
Transform mat4ToTransform(const glm::mat4x4& m);
glm::vec3 transformPoint(const Transform& a, const glm::vec3& b);
glm::vec3 transformVector(const Transform& a, const glm::vec3& b);
glm::quat fromTo(const glm::vec3& from, const glm::vec3& to);
float angle(const glm::vec3& l, const glm::vec3& r);
