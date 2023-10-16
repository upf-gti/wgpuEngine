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

class Mesh {

	std::vector<InterleavedData>	 vertices;

	WGPUBuffer		vertex_buffer   = nullptr;

public:

	~Mesh();

	static WebGPUContext* webgpu_context;
    static Mesh* quad_mesh;

	WGPUBuffer& get_vertex_buffer();

    std::vector<InterleavedData>& get_vertices() { return vertices; }

	void create_quad(float w = 1.f, float h = 1.f, const glm::vec3& color = {1.f, 1.f, 1.f});
	void create_from_vertices(const std::vector<InterleavedData>& _vertices);

    void create_vertex_buffer();

	void* data();
	uint32_t get_vertex_count();
	uint64_t get_byte_size();
};
