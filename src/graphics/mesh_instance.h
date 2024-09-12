#pragma once

#include "material.h"
#include "surface.h"
#include "framework/animation/skeleton.h"
#include <unordered_map>

class Shader;
class Texture;
class Skeleton;

class MeshInstance {

protected:

    Skeleton* skeleton = nullptr;

    bool frustum_culling_enabled = true;

    std::vector<Surface*> surfaces;
    std::unordered_map<Surface*, Material*> material_overrides;

public:

    MeshInstance();
	virtual ~MeshInstance();

    Material* get_surface_material(int surface_idx);

    void set_surface_material_override(Surface* surface, Material* material);

    void set_frustum_culling_enabled(bool enabled);
    bool get_frustum_culling_enabled();

    Material* get_surface_material_override(Surface* surface);

	void  add_surface(Surface* surface);
    const std::vector<Surface*>& get_surfaces() const;
    Surface* get_surface(int surface_idx) const;
    uint32_t get_surface_count() const;

    void set_skeleton(Skeleton* s);
    Skeleton* get_skeleton();
};
