#include "entity_mesh.h"

#include "graphics/renderer.h"
#include "graphics/mesh.h"

void EntityMesh::render()
{
	if (!mesh || !material.shader) return;

	mesh->add_instance_data({ model, material.color });

	if (mesh->get_instances_size() > mesh->get_instances_gpu_size()) {
		mesh->create_bind_group(material.shader, 0, material.diffuse);
	}

	material.shader->get_pipeline()->add_renderable(mesh);
}

void EntityMesh::update(float delta_time)
{

}

void EntityMesh::set_mesh(Mesh* mesh)
{
	this->mesh = mesh;
}
