#include "entity_mesh.h"

#include "graphics/renderer.h"
#include "graphics/mesh.h"

void EntityMesh::render()
{
	if (!mesh || !shader) return;

	instance_id = mesh->get_instances_size();
	mesh->add_instance_data({ model, color });

	if (mesh->get_instances_size() > mesh->get_instances_gpu_size()) {
		mesh->create_bind_group(shader, 0);
	}

	shader->get_pipeline()->add_renderable(mesh);
}

void EntityMesh::update(float delta_time)
{

}

void EntityMesh::set_mesh(Mesh* mesh)
{
	this->mesh = mesh;
	color = mesh->get_color();
}
