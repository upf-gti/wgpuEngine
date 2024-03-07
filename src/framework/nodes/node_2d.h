#pragma once

#include "framework/nodes/node.h"
#include "framework/math.h"
#include "graphics/renderer_storage.h"

#include <string>
#include <vector>

enum Node2DType {
    UNDEFINED,
    TEXT,
    LABEL,
    BUTTON,
    SLIDER,
    COLOR_PICKER,
    GROUP,
    PANEL,
    NUM_2D_TYPES
};

class Node2D : public Node {

protected:

    static unsigned int last_uid;

	glm::mat3x3 model = glm::mat3x3(1.0f);

    glm::vec2 size;

    Node2D* parent = nullptr;

    unsigned int uid = 0;
    uint8_t type = Node2DType::UNDEFINED;

    RendererStorage::sUIData ui_data;

public:

    Node2D() : model(1.0f) {};
    Node2D(const glm::vec2& p, const glm::vec2& s);
	virtual ~Node2D() {};

    virtual void add_child(Node2D* child);
    virtual void remove_child(Node2D* child);

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
    uint8_t get_type() const;

    bool is_hovered();

	void set_translation(const glm::vec2& translation);
    void set_model(const glm::mat3x3& _model);
};
