#pragma once

#include "entity_mesh.h"
#include "graphics/font.h"

class TextEntity : public EntityMesh {

	/*struct vertex {
		vec3 pos;    vec2 uv;    vec4 color;
		vertex(vec3 _pos, vec2 _uv, vec4 _color) :pos(_pos), uv(_uv), color(_color) {};
	};
	std::vector<vertex> m_buffer;*/
	// CMaterial* m_material = nullptr;

	Font* font = nullptr;

	float scale = 1.f;

	void write_char(glm::vec3 pos, Character& c);

public:

	TextEntity() : EntityMesh() {};
	virtual ~TextEntity() {};

	virtual void render() override;
	virtual void update(float delta_time) override;

	/*
	*	Font Rendering
	*/

	int get_text_width(const std::string text);
	void set_scale(float _scale) { scale = _scale; }
	void load(std::string font_name);
	void destroy();
	void write(glm::vec2 pos, glm::vec2 size, std::wstring text, bool wrap = true);
	void write(glm::vec3 pos, glm::vec2 size, std::wstring text, bool wrap = true);
	void write(glm::mat4x4 model, glm::vec2 size, std::wstring text, bool wrap = true);
};
