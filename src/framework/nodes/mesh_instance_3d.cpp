#include "mesh_instance_3d.h"

#include "graphics/renderer.h"
#include "framework/animation/skeleton.h"

MeshInstance3D::MeshInstance3D() : Node3D()
{

}

MeshInstance3D::~MeshInstance3D()
{
    
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

void MeshInstance3D::set_skeleton(Skeleton *s)
{
    skeleton = s;
}

Skeleton* MeshInstance3D::get_skeleton() {
    return skeleton;
}
