#include "entity_mesh.h"

#include "graphics/renderer.h"
#include "graphics/mesh.h"

EntityMesh::EntityMesh() : Entity()
{
    material.type |= MATERIAL_COLOR;
}

void EntityMesh::render()
{
	if (!mesh || !material.shader) return;

    RendererStorage::instance->register_material(Renderer::instance->get_webgpu_context(), material);

    Renderer::instance->add_renderable(this);
}

void EntityMesh::update(float delta_time)
{

}

void EntityMesh::set_mesh(Mesh* mesh)
{
	this->mesh = mesh;
}
