#include "entity_mesh.h"

#include "graphics/renderer.h"

void EntityMesh::render()
{
	Renderer::instance->add_renderable(this);
}

void EntityMesh::update(float delta_time)
{
}
