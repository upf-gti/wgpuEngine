#pragma once

#include <vector>
#include "entity_mesh.h"
#include "graphics/font.h"

struct InterleavedData;

class TextEntity : public EntityMesh {

	std::vector<InterleavedData> vertices;

	Font* font = nullptr;

	float scale = 1.f;
	glm::vec2 box_size;
	bool wrap = true;

	std::string text = "";

	void append_char(glm::vec3 pos, Character& c);

public:

	TextEntity(const std::string& _text, glm::vec2 _box_size, bool _wrap = true);
	virtual ~TextEntity();

	virtual void render() override;
	virtual void update(float delta_time) override;

	/*
	*	Font Rendering
	*/

	int get_text_width(const std::string text);
	void set_scale(float _scale) { scale = _scale; }
	void generate_mesh();
};
