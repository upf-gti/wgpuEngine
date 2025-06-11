#pragma once

#include "material.h"
#include "surface.h"
#include "framework/animation/skeleton.h"
#include <unordered_map>

class Shader;
class Texture;
class Skeleton;
class Node;

class Mesh {

protected:

    std::string mesh_type;

    Node* node_ref = nullptr;
    Skeleton* skeleton = nullptr;

    bool frustum_culling_enabled = true;

    std::vector<Surface*> surfaces;
    std::unordered_map<Surface*, Material*> material_overrides;

    bool receive_shadows = false;

public:

    Mesh();
	virtual ~Mesh();

    Material* get_surface_material(int surface_idx);
    bool get_frustum_culling_enabled();
    bool get_receive_shadows() { return receive_shadows; }
    Material* get_surface_material_override(Surface* surface);
    const std::vector<Surface*>& get_surfaces() const;
    std::vector<Surface*>& get_surfaces();
    Surface* get_surface(int surface_idx) const;
    uint32_t get_surface_count() const;
    Skeleton* get_skeleton();
    Node* get_node_ref() { return node_ref; }
    const std::string& get_mesh_type() { return mesh_type; }

    void set_surface_material_override(Surface* surface, Material* material);
    void set_frustum_culling_enabled(bool enabled);
    void set_receive_shadows(bool receive_shadows) { this->receive_shadows = receive_shadows; }
    void set_node_ref(Node* node) { node_ref = node; }
    void set_skeleton(Skeleton* s);

	void add_surface(Surface* surface);

    virtual void render_gui();
};
