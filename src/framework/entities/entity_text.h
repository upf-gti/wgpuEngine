#pragma once

#include <vector>
#include "entity_mesh.h"
#include "graphics/font.h"
#include "framework/colors.h"

struct InterleavedData;

class TextEntity : public EntityMesh {

	std::vector<InterleavedData> vertices;

	Font* font = nullptr;

	float font_scale = 1.f;
	glm::vec2 box_size;
	glm::vec3 color = colors::WHITE;
	bool wrap = true;

	std::string text = "";

	void append_char(glm::vec3 pos, Character& c);

public:

	TextEntity(const std::string& _text, glm::vec2 _box_size = { 1, 1 }, bool _wrap = false);
	virtual ~TextEntity() {};

	virtual void update(float delta_time) override;

	/*
	*	Font Rendering
	*/

	int get_text_width(const std::string text);
	TextEntity* set_color(const glm::vec3& _color) { color = _color; return this; }
	TextEntity* set_scale(float _scale) { font_scale = _scale; return this; }
	void generate_mesh();
};
