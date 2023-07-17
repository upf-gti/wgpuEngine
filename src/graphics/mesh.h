#pragma once

#include <vector>
#include "includes.h"

#include "graphics/webgpu_context.h"

enum eVertexLayoutDefault {
	POSITION,
	UV,
	NORMAL,
	COLOR,
	DEFAULT_SIZE
};

enum eVertexBufferLayout {
	VB_DEFAULT,
	VB_SIZE
};

enum eBindGroupLayout {
	BG_DEFAULT,
	BG_SIZE
};

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

	Uniform			uniform;

	static Uniform	default_uniform;

	static std::vector<WGPUVertexAttribute> vertex_buffer_attributes[VB_SIZE];
	static WGPUVertexBufferLayout vertex_buffer_layouts[VB_SIZE];

	static WGPUBindGroupLayout bind_group_layouts[BG_SIZE];

	static bool vertex_buffer_layouts_initialized;
	static bool bind_groups_initialized;

	void create_vertex_buffer();
	void create_bind_group();

public:

	~Mesh();

	static WebGPUContext* webgpu_context;

	bool load(const char* filepath);

	static void init_vertex_buffer_layouts();
	static void init_bind_group_layouts();

	static WGPUVertexBufferLayout get_vertex_buffer_layout(eVertexBufferLayout layout_type);
	static WGPUBindGroupLayout get_bind_group_layout(eBindGroupLayout bind_group_type);

	WGPUBuffer& get_vertex_buffer();
	WGPUBindGroup& get_bind_group();

	void create_quad(float w = 1.f, float h = 1.f, const glm::vec3& color = {1.f, 1.f, 1.f});
	void create_from_vertices(const std::vector<InterleavedData>& _vertices);

	void update_model_matrix(const glm::mat4x4& model);
	void update_material_color(const glm::vec3& color);

	void* data();
	size_t get_vertex_count();
	size_t get_byte_size();
};
