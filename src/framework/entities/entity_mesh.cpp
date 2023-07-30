#include "entity_mesh.h"

#include "graphics/renderer.h"
#include "graphics/mesh.h"

void EntityMesh::render()
{
	if (!mesh) return;

	if (mesh->get_instances_dirty()) {
		mesh->create_bind_group(0);
	}

	if (model_dirty) {
		mesh->update_model_matrix(model, instance_id);
		model_dirty = false;
	}

	if (mesh->get_shader()) {
		mesh->get_shader()->get_pipeline()->add_renderable(this);
	}
}

void EntityMesh::update(float delta_time)
{

}

void EntityMesh::set_mesh(Mesh* mesh)
{
	this->mesh = mesh;
	instance_id = mesh->get_instances();
	mesh->add_instance();
}
