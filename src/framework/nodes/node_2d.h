#pragma once

#include "framework/nodes/node.h"
#include "framework/math.h"
#include "graphics/renderer_storage.h"

#include <string>
#include <vector>

enum Node2DClassType {
    SELECTOR_BUTTON,
    SELECTOR,
    TEXT,
    LABEL,
    BUTTON,
    SUBMENU,
    SLIDER,
    COLOR_PICKER,
    GROUP,
    HCONTAINER,
    VCONTAINER,
    PANEL,
    UNDEFINED,
    NUM_2D_TYPES
};

struct sInputData {
    bool is_hovered = false;
    bool is_pressed = false;
    bool was_pressed = false;
    bool was_released = false;
    glm::vec2 local_position = glm::vec2(0.0f);
    float ray_distance = 0.0f;
};

class Node2D : public Node {

protected:

    static unsigned int last_uid;
    static bool propagate_event;

    uint32_t    uid = 0;
    uint8_t     class_type = Node2DClassType::UNDEFINED;
    Node2D*     parent = nullptr;
    bool        visibility = true;
	glm::mat3x3 model = glm::mat3x3(1.0f);
    glm::vec2   size = { 0.0f, 0.0f };

    glm::mat4x4 viewport_model = glm::mat4x4(1.0f);

    RendererStorage::sUIData ui_data;

public:

    Node2D() : Node2D("unnamed", { 0.0f, 0.0f }, { 0.0f, 0.0f }) {};
    Node2D(const std::string& name, const glm::vec2& p, const glm::vec2& s);
	virtual ~Node2D() {};

    virtual void add_child(Node2D* child);
    virtual void remove_child(Node2D* child);
    virtual void on_children_changed();

	virtual void render();
	virtual void update(float delta_time);

    virtual sInputData get_input_data() { return sInputData(); };

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

	void set_translation(const glm::vec2& translation);
    void set_model(const glm::mat3x3& _model);
    void set_visibility(bool value);
    void set_viewport_model(glm::mat4x4 model);
    virtual void set_priority(uint8_t priority);

    static std::map<std::string, Node2D*> all_widgets;

    static Node2D* get_widget_from_name(const std::string& name);
    static void clean();

    static bool must_allow_propagation;

    static void allow_propagation();
    static void stop_propagation();
    static bool should_propagate_event(uint8_t priority);
};
