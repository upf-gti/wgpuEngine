#pragma once

#include "framework/nodes/node.h"

#include <string>
#include <vector>

class Node3D : public Node {

protected:

    glm::mat4x4 model = glm::mat4x4(1.0f);

    Node3D* parent = nullptr;

public:

    Node3D() : model(1.0f) {};
	virtual ~Node3D() {};

    void add_child(Node3D* child);
    void remove_child(Node3D* child);

	virtual void render();
	virtual void update(float delta_time);

	void translate(const glm::vec3& translation);
	void rotate(float angle, const glm::vec3& axis);
    void rotate(const glm::quat& q);
	void scale(glm::vec3 scale);

	const glm::vec3 get_local_translation() const;
    const glm::vec3 get_translation() const;
    virtual glm::mat4x4 get_global_model() const;
    glm::mat4x4 get_model()  const;
    glm::mat4x4 get_rotation() const;
    Node3D* get_parent() const;

    void set_translation(const glm::vec3& translation);
    void set_model(const glm::mat4x4& _model) { model = _model; }
};
