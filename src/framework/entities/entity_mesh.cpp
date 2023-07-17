#include "entity_mesh.h"

#include "graphics/renderer.h"
#include "graphics/mesh.h"

void EntityMesh::render()
{
	if (model_dirty && mesh) {
		mesh->update_model_matrix(model);
		model_dirty = false;
	}

	Renderer::instance->add_renderable(this);
}

void EntityMesh::update(float delta_time)
{
}
