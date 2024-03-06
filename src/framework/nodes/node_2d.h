#pragma once

#include "framework/nodes/node.h"
#include "framework/math.h"

#include <string>
#include <vector>

class Node2D : public Node {

protected:

	glm::mat3x3 model = glm::mat3x3(1.0f);

    Node2D* parent = nullptr;

public:

    Node2D() : model(1.0f) {};
	virtual ~Node2D() {};

    void add_child(Node2D* child);
    void remove_child(Node2D* child);

	/*virtual void render();
	virtual void update(float delta_time);*/

	void translate(const glm::vec2& translation);
	void rotate(float angle);
    void rotate(const glm::quat& q);
	void scale(glm::vec2 scale);

	const glm::vec2 get_local_translation() const;
    const glm::vec2 get_translation() const;
    virtual glm::mat3x3 get_global_model() const;
    glm::mat3x3 get_model() const;
    glm::mat3x3 get_rotation() const;

	void set_translation(const glm::vec2& translation);
    void set_model(const glm::mat3x3& _model);
};
