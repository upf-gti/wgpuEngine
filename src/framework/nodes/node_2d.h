#pragma once

#include "framework/nodes/node.h"

#include "graphics/renderer_storage.h"

#include "glm/glm.hpp"

#include <string>
#include <vector>

enum Node2DClassType {
    CURSOR = 10,
    TEXT,
    TEXT_SHADOW,
    SELECTOR_BUTTON,
    SELECTOR,
    LABEL,
    BUTTON_MARK,
    BUTTON,
    TEXTURE_BUTTON,
    COMBO_BUTTON,
    SUBMENU,
    HSLIDER,
    VSLIDER,
    COLOR_PICKER,
    GROUP,
    COMBO,
    CONTAINER,
    HCONTAINER,
    VCONTAINER,
    IMAGE,
    PANEL_BUTTON,
    PANEL,
    UNDEFINED
};

struct sInputData {
    bool is_hovered = false;
    bool is_pressed = false;
    bool was_hovered = false;
    bool was_pressed = false;
    bool was_released = false;
    glm::vec2 local_position = glm::vec2(0.0f);
    glm::vec3 ray_intersection = glm::vec3(0.0f);
    float ray_distance = -1.0f;
};

class Node2D : public Node {

protected:

    static unsigned int last_uid;

    uint32_t    uid = 0;
    uint8_t     class_type = Node2DClassType::UNDEFINED;
    Node2D*     parent = nullptr;
    bool        visibility = true;
	glm::mat3x3 model = glm::mat3x3(1.0f);

    glm::vec2   size = { 0.0f, 0.0f };
    glm::vec2   scaling = { 1.0f, 1.0f };

    glm::mat4x4 viewport_model = glm::mat4x4(1.0f);

public:

    sUIData ui_data;

    Node2D() : Node2D("unnamed", { 0.0f, 0.0f }, { 0.0f, 0.0f }) {};
    Node2D(const std::string& name, const glm::vec2& p, const glm::vec2& s);
	virtual ~Node2D();

    virtual void add_child(Node2D* child);
    virtual void remove_child(Node2D* child);
    virtual void on_children_changed();
    virtual void update_ui_data() {};

	virtual void render();
	virtual void update(float delta_time);

    void release() override;

    virtual sInputData get_input_data(bool ignore_focus = false) { return sInputData(); };
    virtual bool on_input(sInputData data) { return false; };
    virtual bool on_pressed() { return false; };

	void translate(const glm::vec2& translation);
	void rotate(float angle);
    void rotate(const glm::quat& q);
	void scale(glm::vec2 scale);

    Node2D* get_parent() const { return parent; }
	const glm::vec2 get_local_translation() const;
    const glm::vec2 get_translation() const;
    const glm::vec2 get_scale() const;
    virtual glm::mat3x3 get_global_model() const;
    glm::mat4x4 get_global_viewport_model() const;
    glm::mat3x3 get_model() const;
    glm::mat3x3 get_rotation() const;
    glm::vec2 get_size() const;
    uint8_t get_class_type() const;
    bool get_visibility() const;

    bool set_visibility(bool value);
	void set_translation(const glm::vec2& translation);
    void set_model(const glm::mat3x3& _model);
    void set_viewport_model(glm::mat4x4 model);
    virtual void set_priority(uint8_t priority);

    static std::map<std::string, Node2D*> all_widgets;

    static Node2D* get_widget_from_name(const std::string& name);
    static void clean();
};
