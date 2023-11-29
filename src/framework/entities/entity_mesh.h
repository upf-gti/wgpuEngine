#pragma once

#include "entity.h"
#include "graphics/material.h"

class Mesh;
class Shader;
class Texture;

class EntityMesh : public Entity {

protected:

	Mesh* mesh = nullptr;

    Material material;

public:

	EntityMesh();
	virtual ~EntityMesh() {};

	virtual void render() override;
	virtual void update(float delta_time) override;

	void set_material_color(const glm::vec4& color) { material.color = color; }
    void set_material_diffuse(Texture* diffuse) { material.diffuse = diffuse; material.flags |= MATERIAL_DIFFUSE; }
    void set_material_shader(Shader* shader) { material.shader = shader; }
    void set_material_flag(eMaterialFlags flag) { material.flags |= flag; }
    void set_material_priority(uint8_t priority) { material.priority = priority; }

    Shader* get_material_shader() { return material.shader; }

	void  set_mesh(Mesh* mesh);
	Mesh* get_mesh() { return mesh; }

    Material& get_material() { return material; }
};
