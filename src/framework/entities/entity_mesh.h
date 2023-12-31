#pragma once

#include "entity.h"
#include "graphics/material.h"
#include "graphics/surface.h"

class Mesh;
class Shader;
class Texture;

class EntityMesh : public Entity {

protected:

    std::vector<Surface> surfaces;

public:

	EntityMesh();
	virtual ~EntityMesh() {};

	virtual void render() override;
	virtual void update(float delta_time) override;

    void set_material_color(int surface_idx, const glm::vec4& color);
    void set_material_diffuse(int surface_idx, Texture* diffuse);
    void set_material_shader(int surface_idx, Shader* shader);
    void set_material_flag(int surface_idx, eMaterialFlags flag);
    void set_material_priority(int surface_idx, uint8_t priority);

	void  add_surface(const Surface& surface);
    const std::vector<Surface>& get_surfaces() const { return surfaces; }
    Surface& get_surface(int surface_idx);
};
