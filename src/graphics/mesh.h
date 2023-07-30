#pragma once

#include <map>
#include <vector>
#include <string>

#include "includes.h"

#include "graphics/webgpu_context.h"

class Texture;

struct InterleavedData {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
	glm::vec3 color;
};

class Mesh {

	struct sUniformMeshData {
		glm::mat4x4 model;
		glm::vec4   color;
	};

	std::vector<InterleavedData>	 vertices;

	WGPUBuffer		vertex_buffer = nullptr;
	WGPUBindGroup   bind_group = nullptr;

	Uniform			mesh_data_uniform;
	Uniform			albedo_uniform;
	Uniform			sampler_uniform;

	Shader*			shader = nullptr;

	Texture*		diffuse = nullptr;
	glm::vec3		color = { 1.0f, 1.0f, 1.0f };

	uint16_t		instances = 0;
	bool			instances_dirty = false;

	static std::map<std::string, Mesh*> meshes;

	void create_vertex_buffer();

	bool load(const std::string& mesh_path);

public:

	~Mesh();

	static WebGPUContext* webgpu_context;

	static Mesh* get(const std::string& mesh_path);

	WGPUBuffer& get_vertex_buffer();
	WGPUBindGroup& get_bind_group();

	void set_texture(Texture* texture) { this->diffuse = texture; }

	void create_quad(float w = 1.f, float h = 1.f, const glm::vec3& color = {1.f, 1.f, 1.f});
	void create_from_vertices(const std::vector<InterleavedData>& _vertices);

	void create_bind_group(uint16_t bind_group_id);
	void create_bind_group_color(Shader* shader, uint16_t bind_group_id);
	void create_bind_group_texture(Shader* shader, uint16_t bind_group_id);

	void update_model_matrix(const glm::mat4x4& model, uint16_t instance_id = 0);
	void update_material_color(const glm::vec3& color, uint16_t instance_id = 0);

	Shader* get_shader();

	void     add_instance() { instances++; instances_dirty = true; }
	uint16_t get_instances() { return instances; }

	bool	 get_instances_dirty() { return instances_dirty; }

	void* data();
	size_t get_vertex_count();
	size_t get_byte_size();
};
