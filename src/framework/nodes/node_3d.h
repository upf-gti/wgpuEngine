#pragma once

#include "framework/nodes/node.h"
#include "framework/math/transform.h"

#include <string>
#include <vector>

enum eColliderShape {
    COLLIDER_SHAPE_NONE,
    COLLIDER_SHAPE_AABB,
    COLLIDER_SHAPE_SPHERE,
    COLLIDER_SHAPE_CAPSULE,
    COLLIDER_SHAPE_CUSTOM,
};

class Node3D : public Node {

protected:

    Transform transform = {};

    eColliderShape collider_shape = COLLIDER_SHAPE_AABB;

    bool selected = false;

public:

    Node3D();
    virtual ~Node3D();

    virtual void render() override;
    virtual void update(float delta_time) override;
    virtual void render_gui() override;

    void translate(const glm::vec3& translation);
    void rotate(float angle, const glm::vec3& axis);
    void rotate(const glm::quat& q);
    void rotate_world(const glm::quat& q);
    void scale(glm::vec3 scale);

    virtual void serialize(std::ofstream& binary_scene_file) override;
    virtual void parse(std::ifstream& binary_scene_file) override;

    const glm::vec3 get_local_translation() const;
    const glm::vec3 get_translation();
    virtual glm::mat4x4 get_global_model();
    glm::mat4x4 get_model();
    glm::quat get_rotation() const;
    Transform& get_transform();
    virtual Transform get_global_transform();

    virtual void set_position(const glm::vec3& translation);
    virtual void set_rotation(const glm::quat& rotation);
    virtual void set_scale(const glm::vec3& scale);
    virtual void set_global_transform(const Transform& new_transform);
    void set_transform(const Transform& new_transform);
    void set_transform_dirty(bool value);
    void set_parent(Node3D* node);

    virtual bool test_ray_collision(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& distance, Node3D** out = nullptr);

    void clone(Node* new_node, bool copy = true) override;

    void select();
    void unselect();

    bool is_selected();
    bool is_child_selected();
};
