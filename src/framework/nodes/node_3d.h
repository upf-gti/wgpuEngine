#pragma once

#include "framework/nodes/node.h"
#include "framework/math/transform.h"

#include <string>
#include <vector>

enum EditModes { TRANSLATE, ROTATE, SCALE };

class Node3D : public Node {

protected:

    Node3D* parent = nullptr;

    Transform transform = {};

    bool selected = false;

    int edit_mode = EditModes::ROTATE;

public:

    Node3D();
    virtual ~Node3D();

    void add_child(Node3D* child);
    void remove_child(Node3D* child);

    virtual void render();
    virtual void update(float delta_time);
    void render_gui();

    void translate(const glm::vec3& translation);
    void rotate(float angle, const glm::vec3& axis);
    void rotate(const glm::quat& q);
    void rotate_world(const glm::quat& q);
    void scale(glm::vec3 scale);

    virtual void serialize(std::ofstream& binary_scene_file);
    virtual void parse(std::ifstream& binary_scene_file);

    const glm::vec3 get_local_translation() const;
    const glm::vec3 get_translation();
    virtual glm::mat4x4 get_global_model();
    glm::mat4x4 get_model();
    glm::quat get_rotation() const;
    Node3D* get_parent() const;
    const Transform& get_transform() const;
    int get_edit_mode();

    void set_position(const glm::vec3& translation);
    void set_scale(const glm::vec3& scale);
    void set_transform_dirty(bool value);
    void set_transform(const Transform& new_transform);
    void set_parent(Node3D* node);
    void set_edit_mode(int mode);

    void select();
    void unselect();

    bool is_selected();
    bool is_child_selected();
};
