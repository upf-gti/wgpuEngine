#include "entity_mesh.h"

#include "graphics/renderer.h"
#include "graphics/mesh.h"

EntityMesh::EntityMesh() : Entity()
{
    material.flags |= MATERIAL_COLOR;
}

void EntityMesh::render()
{
    if (mesh && material.shader)
    {
        RendererStorage::instance->register_material(Renderer::instance->get_webgpu_context(), material);
        Renderer::instance->add_renderable(this);
    }

    Entity::render();
}

void EntityMesh::update(float delta_time)
{
    Entity::update(delta_time);
}

void EntityMesh::set_mesh(Mesh* mesh)
{
	this->mesh = mesh;
}
