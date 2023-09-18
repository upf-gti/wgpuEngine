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
	glm::vec3 color = glm::vec3(1.0f);
};

struct sUniformMeshData {
	glm::mat4x4 model;
	glm::vec4   color;
};

class Mesh {

    std::string alias;

	std::vector<InterleavedData>	 vertices;

	WGPUBuffer		vertex_buffer   = nullptr;
	WGPUBindGroup   bind_group      = nullptr;

	Uniform			mesh_data_uniform;
	Uniform			albedo_uniform;
	Uniform			sampler_uniform;

	Texture*		diffuse = nullptr;
	glm::vec4		color = { 1.0f, 1.0f, 1.0f, 1.0f };

	uint32_t		instances_gpu_size = 0;

	std::vector<sUniformMeshData> instance_data;

	static std::map<std::string, Mesh*> meshes;

	void create_vertex_buffer();

    bool load_obj(const std::string& mesh_path);
    bool load_gltf(const std::string& scene_path);

public:

	~Mesh();

	static WebGPUContext* webgpu_context;

	static Mesh* get(const std::string& mesh_path);

	WGPUBuffer& get_vertex_buffer();
	WGPUBindGroup& get_bind_group();

    void set_alias(const std::string& a) { this->alias = a; }
	void set_texture(Texture* texture) { this->diffuse = texture; }

	void create_quad(float w = 1.f, float h = 1.f, const glm::vec3& color = {1.f, 1.f, 1.f});
	void create_from_vertices(const std::vector<InterleavedData>& _vertices);

	void create_bind_group(Shader* shader, uint16_t bind_group_id);
	void create_bind_group_color(Shader* shader, uint16_t bind_group_id);
	void create_bind_group_texture(Shader* shader, uint16_t bind_group_id);

	void update_model_matrix(const glm::mat4x4& model, uint32_t instance_id = 0);
	void update_material_color(const glm::vec3& color, uint32_t instance_id = 0);

	void update_instance_model_matrices();

	void add_instance_data(sUniformMeshData model);

	uint32_t get_instances_size()	  { return static_cast<uint32_t>(instance_data.size()); }
	uint32_t get_instances_gpu_size() { return instances_gpu_size; }
	void	 clear_instances() { instance_data.clear(); }

	glm::vec4 get_color() { return color; }
    const std::string& get_alias() { return alias; };

	void* data();
	uint32_t get_vertex_count();
	uint64_t get_byte_size();
};
