#pragma once

#include "node_3d.h"
#include "graphics/material.h"
#include "graphics/surface.h"

#include <unordered_map>

class Shader;
class Texture;

class MeshInstance3D : public Node3D {

protected:

    std::vector<Surface*> surfaces;
    std::unordered_map<Surface*, Material> material_overrides;

public:

    MeshInstance3D();
	virtual ~MeshInstance3D();

	virtual void render() override;
	virtual void update(float delta_time) override;

    void set_surface_material_color(int surface_idx, const glm::vec4& color);
    void set_surface_material_diffuse(int surface_idx, Texture* diffuse);
    void set_surface_material_shader(int surface_idx, Shader* shader);
    void set_surface_material_flag(int surface_idx, eMaterialFlags flag);
    void set_surface_material_priority(int surface_idx, uint8_t priority);

    void set_surface_material_override_color(int surface_idx, const glm::vec4& color);
    void set_surface_material_override_diffuse(int surface_idx, Texture* diffuse);
    void set_surface_material_override_shader(int surface_idx, Shader* shader);
    void set_surface_material_override_flag(int surface_idx, eMaterialFlags flag);
    void set_surface_material_override_priority(int surface_idx, uint8_t priority);

    void set_surface_material_override(Surface* surface, const Material& material);
    Material* get_surface_material_override(Surface* surface);

	void  add_surface(Surface* surface);
    const std::vector<Surface*>& get_surfaces() const;
    Surface* get_surface(int surface_idx) const;
};
