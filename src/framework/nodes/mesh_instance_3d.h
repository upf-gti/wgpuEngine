#pragma once

#include "node_3d.h"
#include "graphics/mesh.h"

class MeshInstance3D : public Node3D {

protected:
    Mesh* mesh = nullptr;

public:

    bool is_skinned = false;

    MeshInstance3D();
	~MeshInstance3D();

    void update_aabb();
    void set_aabb(const AABB& new_aabb) override;
    void set_surface_material_override(Surface* surface, Material* material);
    void set_frustum_culling_enabled(bool enabled);
    void set_receive_shadows(bool new_receive_shadows);
    void set_mesh(Mesh* new_mesh);

    Mesh* get_mesh() const { return mesh; }
    bool get_frustum_culling_enabled();
    Material* get_surface_material(int surface_idx);
    Material* get_surface_material_override(Surface* surface);
    const std::vector<Surface*>& get_surfaces() const;
    Surface* get_surface(int surface_idx) const;
    uint32_t get_surface_count() const;

    void add_surface(Surface* surface);

	virtual void render() override;
	virtual void update(float delta_time) override;

    void render_gui() override;
};
