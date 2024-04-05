#pragma once

#include "material.h"
#include "surface.h"

#include <unordered_map>

class Shader;
class Texture;

class MeshInstance {

protected:

    std::vector<Surface*> surfaces;
    std::unordered_map<Surface*, Material> material_overrides;

public:

    MeshInstance();
	virtual ~MeshInstance();

    void set_surface_material_color(int surface_idx, const glm::vec4& color);
    void set_surface_material_diffuse(int surface_idx, Texture* diffuse);
    void set_surface_material_shader(int surface_idx, Shader* shader);
    void set_surface_material_flag(int surface_idx, eMaterialFlags flag);
    void set_surface_material_priority(int surface_idx, uint8_t priority);
    void set_surface_material_transparency_type(int surface_idx, eTransparencyType transparency_type);
    void set_surface_material_cull_type(int surface_idx, eCullType cull_type);
    void set_surface_material_topology_type(int surface_idx, eTopologyType topology_type);
    void set_surface_material_depth_read(int surface_idx, bool depth_read);
    void set_surface_material_depth_write(int surface_idx, bool depth_write);

    void set_surface_material_override_color(int surface_idx, const glm::vec4& color);
    void set_surface_material_override_diffuse(int surface_idx, Texture* diffuse);
    void set_surface_material_override_shader(int surface_idx, Shader* shader);
    void set_surface_material_override_flag(int surface_idx, eMaterialFlags flag);
    void set_surface_material_override_priority(int surface_idx, uint8_t priority);
    void set_surface_material_override_transparency_type(int surface_idx, eTransparencyType transparency_type);
    void set_surface_material_override_cull_type(int surface_idx, eCullType cull_type);
    void set_surface_material_override_topology_type(int surface_idx, eTopologyType topology_type);
    void set_surface_material_override_depth_read(int surface_idx, bool depth_read);
    void set_surface_material_override_depth_write(int surface_idx, bool depth_write);

    void set_surface_material_override(Surface* surface, const Material& material);
    Material* get_surface_material_override(Surface* surface);

	void  add_surface(Surface* surface);
    const std::vector<Surface*>& get_surfaces() const;
    Surface* get_surface(int surface_idx) const;
};
