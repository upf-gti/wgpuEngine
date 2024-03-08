#pragma once

#include "framework/nodes/node.h"
#include "framework/math.h"
#include "graphics/renderer_storage.h"

#include <string>
#include <vector>

enum Node2DClassType {
    UNDEFINED,
    TEXT,
    LABEL,
    BUTTON,
    SLIDER,
    COLOR_PICKER,
    GROUP,
    HCONTAINER,
    VCONTAINER,
    SUBMENU,
    PANEL,
    NUM_2D_TYPES
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

	void translate(const glm::vec2& translation);
	void rotate(float angle);
    void rotate(const glm::quat& q);
	void scale(glm::vec2 scale);

	const glm::vec2 get_local_translation() const;
    const glm::vec2 get_translation() const;
    virtual glm::mat3x3 get_global_model() const;
    glm::mat3x3 get_model() const;
    glm::mat3x3 get_rotation() const;
    glm::vec2 get_size() const;
    uint8_t get_class_type() const;
    bool get_visibility() const;

    bool is_hovered();

	void set_translation(const glm::vec2& translation);
    void set_model(const glm::mat3x3& _model);
    void set_visibility(bool value);

    static std::map<std::string, Node2D*> all_widgets;

    static Node2D* get_widget_from_name(const std::string& name);
    static void clean();
};
