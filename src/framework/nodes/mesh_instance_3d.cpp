#include "mesh_instance_3d.h"

#include "graphics/renderer.h"
#include "framework/animation/skeleton.h"

MeshInstance3D::MeshInstance3D() : Node3D()
{
    node_type = "MeshInstance3D";
}

MeshInstance3D::~MeshInstance3D()
{

}

void MeshInstance3D::set_aabb(const AABB& new_aabb)
{
    aabb = new_aabb;
}

void MeshInstance3D::render()
{
    Renderer::instance->add_renderable(this, get_global_model());

    Node3D::render();
}

void MeshInstance3D::update(float delta_time)
{  
    Node3D::update(delta_time);
}
