#pragma once

#include "framework/math.h"

#include "framework/aabb.h"

#include <string>
#include <vector>

class Entity {

protected:
	std::string name;
	glm::mat4x4 model = glm::mat4x4(1.0f);

	Entity* parent = nullptr;
	std::vector<Entity*> children;

    AABB aabb = {};

    // Use to discard either self of its children
    bool process_children = true;
    bool active = true;

public:

	Entity() : model(1.0f) {};
	virtual ~Entity() {};

	void add_child(Entity* child);
	void remove_child(Entity* child);

	virtual void render();
	virtual void update(float delta_time);

	void translate(const glm::vec3& translation);
	void rotate(float angle, const glm::vec3& axis);
    void rotate(const glm::quat& q);
	void scale(glm::vec3 scale);

    void set_name(std::string name) { this->name = name; }
    std::string get_name() const { return name; }

	// Some useful methods

	const glm::vec3 get_local_translation();
    const glm::vec3 get_translation();
    virtual glm::mat4x4 get_global_model() const;
    glm::mat4x4 get_model() { return model; }
    glm::mat4x4 get_rotation();
    std::vector<Entity*>& get_children() { return children; }
    Entity* get_parent() { return parent; }
    bool get_process_children() { return process_children; }
    bool is_active() { return active; }

    void set_aabb(const AABB& aabb) { this->aabb = aabb; }
    AABB get_aabb();

	void set_translation(const glm::vec3& translation);
	void set_model(const glm::mat4x4& _model) { model = _model; }
    void set_process_children(bool value) { process_children = value; }
    void set_active(bool value) { active = value; }
};
