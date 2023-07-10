#pragma once

#include <vector>
#include "includes.h"

#include "graphics/webgpu_context.h"

enum eVertexLayout {
	POSITION,
	UV,
	NORMAL,
	COLOR
};

class Mesh {

	struct InterleavedData {
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 color;
	};

	std::vector<InterleavedData>	 vertices;
	std::vector<WGPUVertexAttribute> vertex_attributes;

	WGPUVertexBufferLayout vertex_buffer_layout = {};
	WGPUBuffer             vertex_buffer = nullptr;

	void create_vertex_buffer();

public:

	~Mesh();

	static WebGPUContext* webgpu_context;

	bool load_mesh(const char* filepath);

	std::vector<WGPUVertexAttribute>&   get_vertex_attributes();
	WGPUVertexBufferLayout&             get_vertex_buffer_layout();
	WGPUBuffer&                         get_vertex_buffer();

	void create_quad();

	void* data();
	size_t get_vertex_count();
	size_t get_byte_size();
};
